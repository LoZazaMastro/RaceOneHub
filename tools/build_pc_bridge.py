from pathlib import Path
import argparse
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--graphics-adapters', action='store_true')
parser.add_argument('--portable-xg', action='store_true', help='Use only XG from the selected FM5 game.')
parser.add_argument('--vc-dir', type=Path)
parser.add_argument('--sdk-dir', type=Path)
parser.add_argument('--sdk-version')
arguments = parser.parse_args()
VC = arguments.vc_dir or Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK = arguments.sdk_dir or Path(r'C:\Program Files (x86)\Windows Kits\10')
SDK_VERSION = arguments.sdk_version or '10.0.26100.0'
OUT = ROOT / 'runtime/pc'
BUILD = ROOT / 'analysis/pc-build'
OUT.mkdir(parents=True, exist_ok=True)
BUILD.mkdir(parents=True, exist_ok=True)

command = [str(VC / 'bin/Hostx64/x64/cl.exe'), '/nologo', '/W4', '/WX',
           '/O2', '/MT', '/LD',
           '/I' + str(VC / 'include')]
for part in ['ucrt', 'shared', 'um', 'winrt']:
    command.append('/I' + str(SDK / 'Include' / SDK_VERSION / part))
sources = [ROOT / 'src/fm5_import_bridge.c', ROOT / 'src/fm5_activation.c',
           ROOT / 'src/file-port/fm5_scene_files.c',
           ROOT / 'src/fm5_language_profile.c',
           ROOT / 'src/native-ui/fm5_native_ui.c',
           ROOT / 'src/native-ui/fm5_native_binding.c',
           ROOT / 'src/native-ui/fm5_native_loading.c',
           ROOT / 'src/save-port/fm5_save_storage.c',
           ROOT / 'src/allocation-port/fm5_language.c',
           ROOT / 'src/fm5_gamepad_diagnostics.c',
           ROOT / 'src/fm5_impulse_triggers.c',
           ROOT / 'src/fm5_impulse_wgi.cpp',
           ROOT / 'src/fm5_bridge_main.c',
           ROOT / 'src/graphics-port/fm5_frame_timing.c',
           ROOT / 'src/graphics-port/fm5_render_cadence.c',
           ROOT / 'src/graphics-port/fm5_queue_diagnostics.cpp',
           ROOT / 'src/graphics-port/fm5_color_observer.cpp',
           ROOT / 'src/allocation-port/fm5_pso_observer.cpp',
           ROOT / 'src/allocation-port/fm5_msaa4.c',
           ROOT / 'src/allocation-port/fm5_luminance_readback.cpp',
           ROOT / 'src/graphics-port/fm5_presentation.c',
           ROOT / 'src/graphics-port/fm5_d3d12_diagnostics.cpp',
           ROOT / 'src/graphics-port/fm5_scissor_state.c',
           ROOT / 'src/fm5_apu_probe.c',
           ROOT / 'src/fm5_scissor_probe.c',
           ROOT / 'src/fm5_msaa_native_probe.cpp',
           ROOT / 'src/audio-tls/apu-port/fm5_apu.c',
           ROOT / 'src/audio-tls/shape-port/fm5_shape.c',
           ROOT / 'src/graphics-port/fm5_graphics_bridge.c',
           ROOT / 'src/audio-tls/fm5_audio_bridge.c',
           ROOT / 'src/profile-port/fm5_manifest_bridge.c']
objects = []
if arguments.portable_xg:
    if not arguments.graphics_adapters:
        parser.error('--portable-xg requires --graphics-adapters')
    command += ['/DFM5_USE_PORTABLE_XG']
    sources.append(ROOT / 'src/fm5_portable_xg.c')
if arguments.graphics_adapters:
    command += ['/DFM5_ENABLE_ABI_ADAPTERS', '/DFM5_GRAPHICS_PRODUCTION_INTEROP']
    sources += [ROOT / path for path in [
        'src/fm5_qi_bridge.c', 'src/fm5_completion_policy.c', 'src/fm5_topology_policy.c',
        'src/fm5_vtable_bridge.c', 'src/fm5_resource_lifetime.c', 'src/fm5_host_xg.c',
        'src/graphics-port/fm5_dxgi_bridge.c',
        'src/graphics-port/fm5_fmask_bridge.c',
        'src/graphics-port/fm5_performance_context.c',
        'src/audio-tls/performance-device/fm5_performance_device.c',
        'src/audio-tls/private-data/fm5_private_data.c',
        'src/audio-tls/resource-port/fm5_resource.c',
        'src/allocation-port/fm5_allocation.c',
        'src/fm5_guest_tls.c', 'src/fm5_tls_boundary.c']]
    assembly = ROOT / 'src/audio-tls/performance-device/device_forwarders.asm'
    object_file = BUILD / 'device_forwarders.obj'
    assemble = subprocess.run([str(VC / 'bin/Hostx64/x64/ml64.exe'), '/nologo', '/c',
                               '/Fo' + str(object_file), str(assembly)],
                              text=True, capture_output=True, cwd=BUILD)
    (BUILD / 'assembler.log').write_text(assemble.stdout + assemble.stderr)
    assemble.check_returncode()
    objects.append(object_file)
command += [str(path) for path in sources + objects]
command += [
            '/Fo' + str(BUILD) + '\\',
            '/Fe' + str(OUT / 'ForzaMastro5Bridge.dll'), '/link', '/Brepro',
            '/LIBPATH:' + str(VC / 'lib/x64'),
            '/LIBPATH:' + str(SDK / 'Lib' / SDK_VERSION / 'ucrt/x64'),
            '/LIBPATH:' + str(SDK / 'Lib' / SDK_VERSION / 'um/x64'),
            'kernel32.lib', 'user32.lib', 'shell32.lib', 'ole32.lib', 'uuid.lib', 'bcrypt.lib',
            '/MAP:' + str(BUILD / 'bridge.map'),
            '/IMPLIB:' + str(BUILD / 'ForzaMastro5Bridge.lib')]
result = subprocess.run(command, text=True, capture_output=True, cwd=BUILD)
(BUILD / 'compiler.log').write_text(result.stdout + result.stderr)
if result.returncode:
    raise SystemExit(result.stdout + result.stderr)
dll = OUT / 'ForzaMastro5Bridge.dll'
(BUILD / 'build.json').write_text(json.dumps({
    'command': command, 'exit_code': result.returncode,
    'sha256': hashlib.sha256(dll.read_bytes()).hexdigest(),
    'bytes': dll.stat().st_size,
    'graphics_adapters': arguments.graphics_adapters,
    'portable_xg': arguments.portable_xg,
    'sources': [str(path.relative_to(ROOT)) for path in sources],
    'game_executed': False,
}, indent=2) + '\n')
print(f'Built {dll} ({dll.stat().st_size} bytes)')
