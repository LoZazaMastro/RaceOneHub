from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import ctypes as C
import hashlib
import json
import os
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'analysis/allocation-port/msaa-reference'
add_python_libraries(__file__)
import pefile
vc = Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
sdk = Path(r'C:\Program Files (x86)\Windows Kits\10')
version = '10.0.26100.0'
env = os.environ.copy()
env['INCLUDE'] = ';'.join(map(str, [vc/'include', *[sdk/'Include'/version/p for p in ['ucrt', 'shared', 'um', 'winrt']]]))
env['LIB'] = ';'.join(map(str, [vc/'lib/x64', *[sdk/'Lib'/version/p/'x64' for p in ['ucrt', 'um']]]))
cl = str(vc / 'bin/Hostx64/x64/cl.exe')
def compile(name, sources, extra=(), libraries=()):
    command = [cl, '/nologo', '/W4', '/WX', '/O2', '/MT', *extra,
               *map(str, sources), '/Fo'+str(OUT)+'\\', '/Fe'+str(OUT/name),
               '/link', 'kernel32.lib', *libraries]
    result = subprocess.run(command, env=env, capture_output=True, text=True)
    (OUT/(name+'.build.log')).write_text(result.stdout+result.stderr)
    assert result.returncode == 0, result.stdout+result.stderr
def execute(command):
    result = subprocess.run(list(map(str, command)), capture_output=True, text=True, timeout=20)
    assert result.returncode == 0, (command, result.returncode, result.stdout, result.stderr)
    return result.stdout, result.stderr

game = ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'
assert hashlib.sha256(game.read_bytes()).hexdigest() == '94dad21851d335e9a33231dc2a1de2ecd4b93eefdf5f6772085bf6886bf47db7'
image = pefile.PE(str(game)).get_memory_mapped_image()
rvas = [0x11d41d0, 0x11d9180, 0x14a6140, 0x14ab738, 0x171a050, 0x171ff0c, 0x18a68b0, 0x18aceb0]
for rva in rvas:
    size = struct.unpack_from('<I', image, rva+24)[0]
    (OUT/f'original-{rva:x}.dxbc').write_bytes(image[rva:rva+size])
module = ROOT/'src/allocation-port/fm5_msaa4.c'
compile('test-msaa4.exe', [module, ROOT/'src/allocation-port/test_msaa4.c'], ['/DFM5_MSAA4_TEST'])
cpu_stdout, cpu_stderr = execute([OUT/'test-msaa4.exe', OUT/'original-11d41d0.dxbc',
                                OUT/'original-14a6140.dxbc', OUT/'selected-production-four-samples.dxbc'])
compile('test-msaa4-fmask-interop.exe', [module, ROOT/'src/allocation-port/test_msaa4_fmask_interop.c',
        ROOT/'src/graphics-port/fm5_fmask_bridge.c',ROOT/'src/audio-tls/resource-port/fm5_resource.c',
        ROOT/'src/allocation-port/fm5_allocation.c'], ['/DFM5_MSAA4_TEST','/DFM5_FMASK_TEST','/DFM5_RESOURCE_TEST'])
interop_stdout, interop_stderr = execute([OUT/'test-msaa4-fmask-interop.exe'])
compile('msaa4-selector-test.dll', [module], ['/LD', '/DFM5_MSAA4_TEST'], ['/EXPORT:FM5SelectFourSampleResolve', '/EXPORT:FM5SetMsaaTestHost'])
dll = C.WinDLL(str(OUT/'msaa4-selector-test.dll'))
select = dll.FM5SelectFourSampleResolve
select.argtypes = [C.c_void_p, C.c_size_t, C.c_uint, C.POINTER(C.c_void_p), C.POINTER(C.c_size_t)]
select.restype = C.c_long
def chunks(data):
    result = {}
    for i in range(struct.unpack_from('<I', data, 28)[0]):
        offset = struct.unpack_from('<I', data, 32+4*i)[0]
        tag, size = struct.unpack_from('<4sI', data, offset)
        result[tag] = (offset+8, size, data[offset+8:offset+8+size])
    return result
families, mutations = [], 0
for index, rva in enumerate(rvas):
    original = (OUT/f'original-{rva:x}.dxbc').read_bytes()
    buffer = C.create_string_buffer(original)
    pointer, size = C.c_void_p(), C.c_size_t()
    assert select(buffer, len(original), 2, C.byref(pointer), C.byref(size)) == 1
    assert pointer.value == C.addressof(buffer) and size.value == len(original)
    assert select(buffer, len(original), 4, C.byref(pointer), C.byref(size)) == 0
    translated = C.string_at(pointer, size.value)
    before, after = chunks(original), chunks(translated)
    assert before[b'ISGN'][2] == after[b'ISGN'][2]
    assert before[b'OSGN'][2] == after[b'OSGN'][2]
    if index % 2 == 0:
        for tag in [b'RDEF', b'ISGN', b'OSGN', b'SHEX']:
            offset, count, _ = before[tag]
            for n in range(count):
                byte = C.c_ubyte.from_address(C.addressof(buffer)+offset+n)
                byte.value ^= 1
                assert select(buffer, len(original), 4, C.byref(pointer), C.byref(size)) != 0
                assert pointer.value == C.addressof(buffer) and size.value == len(original)
                byte.value ^= 1
                mutations += 1
    output_name = 'selected-production-four-samples-tonemap.dxbc' if index >= 4 else 'selected-production-four-samples.dxbc'
    (OUT/output_name).write_bytes(translated)
    families.append({'rva': hex(rva), 'original_bytes': len(original), 'translated_bytes': len(translated),
                     'translated_sha256': hashlib.sha256(translated).hexdigest(),
                     'input_output_signature_exact': True, 'two_sample_path_unchanged': True})
compile('probe-original-resolve-four-samples.exe', [OUT/'probe-original-resolve-four-samples.cpp'],
        ['/EHsc'], ['d3d11.lib', 'd3dcompiler.lib'])
identity_stdout, identity_stderr = execute([OUT/'probe-original-resolve-four-samples.exe',
    OUT/'original-11d41d0.dxbc', OUT/'selected-production-four-samples.dxbc'])
compile('probe-tonemap-four-samples.exe', [OUT/'probe-tonemap-four-samples.cpp'],
        ['/EHsc'], ['d3d11.lib', 'd3dcompiler.lib'])
tone_stdout, tone_stderr = execute([OUT/'probe-tonemap-four-samples.exe',
    OUT/'original-171a050.dxbc', OUT/'original-18a68b0.dxbc', OUT/'selected-production-four-samples-tonemap.dxbc'])
compile('probe-four-sample-mask.exe', [module, OUT/'probe-four-sample-mask.cpp'],
        ['/EHsc','/DFM5_MSAA4_TEST','/I'+str(module.parent)], ['d3d11.lib','d3dcompiler.lib'])
mask_stdout, mask_stderr = execute([OUT/'probe-four-sample-mask.exe', OUT/'selected-production-four-samples.dxbc'])
assert len(mask_stdout.splitlines()) == 21
import runpy
mask_reference = runpy.run_path(str(module.parent/'test_sample_masks.py'))['verify'](dll)
report = {'passed': True, 'semantic_shader_mutation_refusals': mutations, 'families': families,
          'cpu_stdout': cpu_stdout, 'cpu_stderr': cpu_stderr,
          'actual_metadata_fmask_interop_stdout': interop_stdout, 'interop_stderr': interop_stderr,
          'identity_HDR_alpha_gpu_stdout': identity_stdout, 'identity_gpu_stderr': identity_stderr,
          'tonemap_HDR_alpha_gpu_stdout': tone_stdout, 'tonemap_gpu_stderr': tone_stderr,
          'tonemap_original_four_fragment_vs_translation_rgba_bitwise_equal': True,
          'partial_sample_mask_gpu_cases': 20, 'exact_default_ffffffff_mask_gpu_cases': 1, 'sample_mask_gpu_stdout': mask_stdout,
          'sample_mask_gpu_stderr': mask_stderr, 'native_original_mask_equivalence': mask_reference,
          'gpu_backend': 'System D3D11 real hardware, four distinct RGBA32_FLOAT samples',
          'production_selector_bytecode_executed': True,
          'game_launched': False, 'runtime_files_changed': False,
          'limitation': 'Actual Az resource/translation acceptance is checked separately in the root-owned native device probe; no game visual claim.'}
(OUT/'complete-msaa4-verification.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps({k: report[k] for k in ['passed', 'semantic_shader_mutation_refusals',
      'identity_HDR_alpha_gpu_stdout', 'tonemap_HDR_alpha_gpu_stdout']}))
