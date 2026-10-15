from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_path
from pc_icon_resources import embed_icon
from fm5_game_files import stage_scene_overrides
from native_branding import apply_native_branding, embed_native_branding_resources
from native_draw_observer_recipe import validate as validate_draw_observer
from native_copytexture_observer_recipe import replacement_plan as copy_observer_plan
from fm5_msaa_hook import (antialiasing_reader_code, antialiasing_validator_leaf,
    antialiasing_validator_patch, VALIDATOR_RVA, VALIDATOR_ERROR_RVA, VALIDATOR_BEFORE)
import argparse
import hashlib
import json
import shutil
import struct
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--graphics-adapters', action='store_true')
parser.add_argument('--d3d12-diagnostics', action='store_true')
parser.add_argument('--gamepad-diagnostics', action='store_true')
parser.add_argument('--frame-timing', action='store_true')
parser.add_argument('--queue-diagnostics', action='store_true')
parser.add_argument('--preset', choices=['low', 'default', 'high', 'ultra'], default='default')
parser.add_argument('--antialiasing', choices=['msaax2', 'msaax4', 'msaax8'], default='msaax2')
parser.add_argument('--frame-rate', type=int, choices=[30, 60, 120], default=60)
arguments = parser.parse_args()
if arguments.d3d12_diagnostics and not arguments.graphics_adapters:
    parser.error('--d3d12-diagnostics requires --graphics-adapters')
add_python_libraries(__file__)
import pefile

SOURCE = ROOT / 'runtime/azimuth-0.9.9-original'
OUT = ROOT / 'runtime/pc'
BUILD = ROOT / 'analysis/pc-build'
BUILD.mkdir(parents=True, exist_ok=True)
GAME = ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe'
PLAY_GAME = GAME
EXPECTED = '579ed8d8b0b16cbf7890380cb7b925c972b837631ede367eada55eae4afc39bc'
original = (SOURCE / 'azimuth.exe').read_bytes()
assert hashlib.sha256(original).hexdigest() == EXPECTED
icon = ROOT / 'launcher/ForzaMastro.Launcher/Assets/ForzaMotorsport5.ico'
icon_staging = ROOT / 'analysis/pc-build/icon-staging.exe'
icon_staging.write_bytes(original)
embed_icon(icon_staging, icon)
embed_native_branding_resources(icon_staging)
icon_data = icon_staging.read_bytes()
original_pe = pefile.PE(data=original)
pe = pefile.PE(data=icon_data)
for section in original_pe.sections:
    if section.Name.rstrip(b'\0') not in [b'.rsrc', b'.reloc']:
        assert pe.get_data(section.VirtualAddress, section.SizeOfRawData) == original_pe.get_data(section.VirtualAddress, section.SizeOfRawData)
data = bytearray(icon_data)
changes = []

def patch(rva, before, after, reason):
    assert len(before) == len(after)
    offset = pe.get_offset_from_rva(rva)
    assert data[offset:offset + len(before)] == before, (hex(rva), reason)
    data[offset:offset + len(before)] = after
    changes.append({'rva': hex(rva), 'before': before.hex(), 'after': after.hex(), 'reason': reason})

def scalar(rva, value, width, reason):
    offset = pe.get_offset_from_rva(rva)
    patch(rva, bytes(data[offset:offset + width]), value.to_bytes(width, 'little'), reason)

profile = 0x2a1eb0
fields = {int(v['offset'], 16): v for v in json.loads(
    (ROOT / 'recipes/native-build-profiles.json').read_text())[0]['fields']}

def profile_string(field, value, wide=False):
    item = fields[field]
    old = item['wide'] if wide else item['ascii']
    encoding = 'utf-16le' if wide else 'ascii'
    assert len(old) == len(value), (hex(field), old, value)
    patch(int(item['target_rva'], 16), old.encode(encoding), value.encode(encoding),
          f'FM5 identity field {field:#x}')

metadata = json.loads((ROOT / 'recipes/fm5-profile-metadata.json').read_text())
assert metadata['passed']
for item in metadata['checksum_fields']:
    source = GAME / item['path']
    actual = hashlib.sha256(source.read_bytes()).hexdigest()
    assert actual == item['fm5_sha256']
    profile_string(int(item['field'], 16), actual)
profile_string(0, '7b35d917', wide=True)
profile_string(8, '1.0.0.20')
profile_string(0x10, hashlib.sha256((GAME / 'Mount/AppxManifest.xml').read_bytes()).hexdigest())
for field, value in metadata['fm5_scalar_fields'].items():
    scalar(profile + int(field, 16), value['decimal'],
           4 if field in ['0x98', '0x9c'] else 8, f'FM5 scalar {field}')

# FH2-only sites are unreachable through the new FM5 dispatcher.
tail_start, tail_end = profile + 0xa8, profile + 0x350
tail_offset = pe.get_offset_from_rva(tail_start)
patch(tail_start, bytes(data[tail_offset:tail_offset + tail_end - tail_start]),
      bytes(tail_end - tail_start), 'Remove every FH2-specific game patch address')
removed_relocations = []
for block in pe.DIRECTORY_ENTRY_BASERELOC:
    for entry in block.entries:
        if tail_start <= entry.rva < tail_end and entry.type:
            struct.pack_into('<H', data, entry.struct.get_file_offset(), 0)
            removed_relocations.append(hex(entry.rva))

for item in json.loads((ROOT / 'recipes/tls-patches.json').read_text())['patches']:
    patch(item['rva'], bytes.fromhex(item['before_hex']), bytes.fromhex(item['after_hex']), item['reason'])
for item in json.loads((ROOT / 'recipes/xg-layout-patches.json').read_text())['patches']:
    patch(int(item['instruction_rva'], 16), bytes.fromhex(item['guard']),
          bytes.fromhex(item['replacement']), item['purpose'])
patch(0x15772c, bytes.fromhex('4c8d35e5511600'), bytes.fromhex('4c8d35ad511600'),
      'FM5 Live link table: retain eight SDK-present entries; omit FH2-only PartyConfig requirement')

for rva, before, after, reason in [
    (0xaeb5e, 0x500000, 0x4000000, 'Accept original FM5 APU heap1 capacity'),
    (0xaeb6a, 0x40000, 0x114000, 'Accept original FM5 APU heap2 capacity'),
    (0xaeb75, 0x500000, 0x4000000, 'Allocate original FM5 APU heap1 capacity'),
    (0xaeb96, 0x40000, 0x114000, 'Allocate original FM5 APU heap2 capacity'),
    (0xaebe7, 0x500000, 0x4000000, 'Expose actual APU heap1 capacity in native range metadata'),
    (0xaec02, 0x40000, 0x114000, 'Expose actual APU heap2 capacity in native range metadata'),
    (0xaec0c, 0x2000000, 0x5000000, 'Keep heap2 logical APU range disjoint from expanded heap1'),
]:
    patch(rva, struct.pack('<I', before), struct.pack('<I', after), reason)

forced_four = json.loads((ROOT / 'recipes/forced-sample-normalizers.json').read_text())
sample_mask_hook = json.loads((ROOT / 'recipes/sample-mask-hook.json').read_text())
assert forced_four['immutable_Az_sha256'] == EXPECTED
assert pe.get_data(int(sample_mask_hook['rva'], 16), sample_mask_hook['bytes']) == bytes.fromhex(sample_mask_hook['original'])
for item in forced_four['patches']:
    assert pe.get_data(int(item['before_guard_rva'], 16), len(bytes.fromhex(item['before_guard']))) == bytes.fromhex(item['before_guard'])
for item in forced_four['patches']:
    patch(int(item['rva'], 16), bytes.fromhex(item['original']), bytes.fromhex(item['replacement']),
          'Use genuine forced sample normalization for selected MSAA4: ' + item['purpose'])

align = lambda n, a: (n + a - 1) // a * a
data_rva = align(pe.sections[-1].VirtualAddress + pe.sections[-1].Misc_VirtualSize,
                 pe.OPTIONAL_HEADER.SectionAlignment)
code_rva = data_rva + 0x1000
payload = bytearray()
imports = pe.OPTIONAL_HEADER.DATA_DIRECTORY[1]
old_imports = pe.get_data(imports.VirtualAddress, len(pe.DIRECTORY_ENTRY_IMPORT) * 20)
payload.extend(old_imports)
descriptor_offset = len(payload)
payload.extend(bytes(40))

def store(blob, alignment=1):
    payload.extend(bytes(align(len(payload), alignment) - len(payload)))
    offset = len(payload)
    payload.extend(blob)
    return data_rva + offset

import_names = ['FM5ResolveImport', 'FM5ApplyPatchesWithLanguage', 'FM5CollectNetworkTemplate']
if arguments.graphics_adapters:
    import_names += ['FM5GraphicsQueryInterface', 'FM5InitializeResourceVtable', 'FM5DestroyLegacyResource',
                     'FM5CreateHostTextureComputer', 'FM5BindGuestTLS', 'FM5ExecuteShapeGraph']
if arguments.d3d12_diagnostics:
    import_names.append('FM5DiagnosticD3D12CreateDevice')
    import_names.append('FM5CreateGraphicsPsoObserved')
if arguments.gamepad_diagnostics:
    import_names.append('FM5SetGamepadVibration')
if arguments.frame_timing:
    import_names += ['FM5FrameLatencyWait', 'FM5PacingTimerWait', 'FM5PacingDeadline']
if arguments.queue_diagnostics:
    import_names.append('FM5DiagnosticExecuteCommandLists')
    import_names.append('FM5ReadbackCopyTextureRegion')
import_names.append(sample_mask_hook['export'])
import_names.append('FM5SetTextLanguageAfterProfile')
import_names.append('FM5RunPreGame')
import_names.append('FM5CreateGameFileW')
import_names.append('FM5ParseNativeAntialiasing')
import_names += ['FM5DrawPreparation', 'FM5DrawPreparationComplete', 'FM5BuildPersistentSaveTitlePath']
import_names.append('FM5PaintOriginalPreparation')
if arguments.graphics_adapters:
    import_names.append('FM5PrepareGraphicsObserved')
dll_name = store(b'ForzaMastro5Bridge.dll\0')
names = [store(b'\0\0' + name.encode() + b'\0', 2)
         for name in import_names]
ilt = store(struct.pack('<' + 'Q' * (len(names) + 1), *names, 0), 8)
iat = store(struct.pack('<' + 'Q' * (len(names) + 1), *names, 0), 8)
struct.pack_into('<IIIII', payload, descriptor_offset, ilt, 0, 0, dll_name, iat)
code = bytearray()
for index in range(len(names)):
    stub_rva = code_rva + len(code)
    code.extend(b'\xff\x25' + struct.pack('<i', iat + index * 8 - stub_rva - 6))

# Keep the host's CreateFile ABI and all arguments. Only the bridge's six
# exact source scene paths can resolve to local overrides; assets stay external.
scene_file_sites = [0x2cdbd, 0x2f657, 0x733d8, 0x735bb, 0x73726, 0x74433,
    0x76e3d, 0xba6df, 0xba97b, 0xbaca2, 0x16c0b3, 0x16c104, 0x16c766,
    0x172fe8, 0x18d316, 0x18d385, 0x1a047d, 0x221b03, 0x24928e, 0x249348]
scene_file_iat = iat + import_names.index('FM5CreateGameFileW') * 8
for site in scene_file_sites:
    patch(site, b'\xff\x15' + struct.pack('<i', 0x26f2f8-site-6),
          b'\xff\x15' + struct.pack('<i', scene_file_iat-site-6),
          'Read original game folder with separate six-file scene overrides')

# Original numeric Settings+4 writer already serializes msaaxN. Extend only
# its INI reader; preserve original cleanup and the current value on unknowns.
aa_start, aa_end = 0x7e2ec, 0x7e3fc
aa_stub = code_rva + import_names.index('FM5ParseNativeAntialiasing') * 6
aa_code = antialiasing_reader_code(aa_start, aa_end, aa_stub)
patch(aa_start, original_pe.get_data(aa_start, aa_end-aa_start), aa_code,
      'Parse exact off/msaax2/msaax4/msaax8 into genuine native Settings sample count')

locale_stub = code_rva + import_names.index('FM5SetTextLanguageAfterProfile') * 6
patch(0x6925, bytes.fromhex('e8c6420300'),
      b'\xe8' + struct.pack('<i', locale_stub - 0x6925 - 5),
      'Preserve native text-language assignment then select an existing FM5 prefix before guest entry')

menu_stub = code_rva + import_names.index('FM5RunPreGame') * 6
patch(0x5e13, bytes.fromhex('e888240100'),
      b'\xe8' + struct.pack('<i', menu_stub - 0x5e13 - 5),
      'Use native fullscreen pre-game settings in the same game window and process')

preparation_paint_stub = code_rva + import_names.index('FM5PaintOriginalPreparation') * 6
for callsite, guard in [(0x1b354, 'e857980200'), (0x1c6ff, 'e8ac840200')]:
    patch(callsite, bytes.fromhex(guard),
          b'\xe8' + struct.pack('<i', preparation_paint_stub-callsite-5),
          'Preserve genuine preparation painting and append same-window loading overlay')

save_stub = code_rva + import_names.index('FM5BuildPersistentSaveTitlePath') * 6
patch(0x1870cb, bytes.fromhex('e8f0acefff'),
      b'\xe8' + struct.pack('<i', save_stub - 0x1870cb - 5),
      'Keep per-user game saves independent of the generated executable folder')
for slot, original_callback, export in [(0x29d980, 0x4a060, 'FM5DrawPreparation'),
                                       (0x29d948, 0x4a000, 'FM5DrawPreparationComplete')]:
    assert any(entry.rva == slot and entry.type == 10
               for block in pe.DIRECTORY_ENTRY_BASERELOC for entry in block.entries)
    callback_stub = code_rva + import_names.index(export) * 6
    patch(slot, struct.pack('<Q', pe.OPTIONAL_HEADER.ImageBase + original_callback),
          struct.pack('<Q', pe.OPTIONAL_HEADER.ImageBase + callback_stub),
          'Reskin native preparation painting while retaining real progress and completion')

if arguments.gamepad_diagnostics:
    setter_entry = 0x2a92a8
    assert any(entry.rva == setter_entry and entry.type == 10
               for block in pe.DIRECTORY_ENTRY_BASERELOC for entry in block.entries)
    setter_stub = code_rva + import_names.index('FM5SetGamepadVibration') * 6
    image_base = pe.OPTIONAL_HEADER.ImageBase
    patch(setter_entry, struct.pack('<Q', image_base + 0xbed60),
          struct.pack('<Q', image_base + setter_stub),
          'Observe native Gamepad vibration requests through an opt-in forwarding diagnostic')

if arguments.frame_timing:
    for call_rva, guard, name in [
        (0x5088e, 'ff1524ea2100', 'FM5FrameLatencyWait'),
        (0x50bdb, 'ff15d7e62100', 'FM5PacingTimerWait'),
        (0x50b73, 'e8b8f2ffff', 'FM5PacingDeadline'),
    ]:
        stub = code_rva + import_names.index(name) * 6
        before = bytes.fromhex(guard)
        after = b'\xe8' + struct.pack('<i', stub - call_rva - 5)
        after += b'\x90' * (len(before) - len(after))
        patch(call_rva, before, after, 'Observe native pacing without changing waits or deadlines: ' + name)

for call_rva, expected, index in [(0x3af4f, 'e83cf3ffff', 0), (0x3ceac, 'e8ff260000', 1),
                                 (0x85c6f, 'e83ce5ffff', 2)]:
    replacement = b'\xe8' + struct.pack('<i', code_rva + index * 6 - call_rva - 5)
    patch(call_rva, bytes.fromhex(expected), replacement, 'Route through FM5 bridge')

if arguments.graphics_adapters:
    draw_observer_recipe = json.loads((ROOT / 'analysis/graphics-port/native-draw-observer-recipe.json').read_text())
    validate_draw_observer(data, draw_observer_recipe, require_original=False)
    prepare_stub = code_rva + import_names.index('FM5PrepareGraphicsObserved') * 6
    for site in draw_observer_recipe['callsites']:
        callsite = int(site['rva'], 16)
        patch(callsite, bytes.fromhex(site['original_call']),
              b'\xe8' + struct.pack('<i', prepare_stub - callsite - 5),
              'Observe prepared native graphics state without changing its returned command context: ' + site['kind'])
    if arguments.d3d12_diagnostics:
        patch(0x567df, bytes.fromhex('ff1583902100'),
              b'\xe8' + struct.pack('<i', code_rva + 9 * 6 - 0x567df - 5) + b'\x90',
              'Enable per-process D3D12 diagnostics before device creation when explicitly requested')
        pso_stub_rva = code_rva + len(code)
        code.extend(bytes.fromhex('498bd5498bcc'))
        code.extend(b'\xff\x25' + struct.pack('<i',
            iat + import_names.index('FM5CreateGraphicsPsoObserved') * 8 - pso_stub_rva - 12))
        patch(0x1364b9, bytes.fromhex('498bd5498bccffd0'),
              b'\xe8' + struct.pack('<i', pso_stub_rva - 0x1364b9 - 5) + b'\x90' * 3,
              'Record exact failing pipeline sample descriptor and actual device capabilities without altering the call')
    proof = json.loads((ROOT / 'analysis/audio-tls/tls-boundary/guest-vector-cfg-proof.json').read_text())
    assert proof['total_sites'] == 327
    assert proof['game_sha256'] == '94dad21851d335e9a33231dc2a1de2ecd4b93eefdf5f6772085bf6886bf47db7'
    assert proof['classification_counts'] == {'safe_vector0_only': 325, 'exact_preserving_double_load': 2}
    patch(0x3cf05, bytes.fromhex('e896170800'),
          b'\xe8' + struct.pack('<i', code_rva + 7 * 6 - 0x3cf05 - 5),
          'Bind and isolate FM5 hard-coded static TLS accesses after the original helper binder')
    patch(0xb054f, bytes.fromhex('e87cf60d00'),
          b'\xe8' + struct.pack('<i', code_rva + 8 * 6 - 0xb054f - 5),
          'Translate proven old-SDK SHAPE graph commands before genuine native execution')
    xg_stub_rva = code_rva + len(code)
    # Replay the original argument setup and carry the resolved XG factory in R8.
    code.extend(bytes.fromhex('488b d7 498b ce 4989 d8 ff25'.replace(' ', '')) +
                struct.pack('<i', iat + 6 * 8 - xg_stub_rva - 15))
    patch(0x9fd75, bytes.fromhex('488bd7498bceffd3'),
          b'\xe8' + struct.pack('<i', xg_stub_rva - 0x9fd75 - 5) + bytes.fromhex('909090'),
          'Use typed modern host XG while guest retains its original SDK XG')
    qi_rva = 0xf3700
    patch(qi_rva, bytes.fromhex('488d055980feff'),
          b'\x48\x8d\x05' + struct.pack('<i', code_rva + 3 * 6 - qi_rva - 7),
          'Select old-SDK graphics interface adapters without changing native vtable identities')
    for call_rva, expected in [(0xf48f3, 'e8d8edffff'), (0xf495f, 'e86cedffff'), (0xf49ef, 'e8dcecffff'),
                               (0xf4a13, 'e8b8ecffff'), (0xf4a37, 'e894ecffff'),
                               (0xf4a5b, 'e870ecffff'), (0xf4a7f, 'e84cecffff')]:
        patch(call_rva, bytes.fromhex(expected),
              b'\xe8' + struct.pack('<i', code_rva + 4 * 6 - call_rva - 5),
              'Initialize typed resource, shader or DXGI factory compatibility methods')
    for call_rva, expected in [(0x5061b, 'e8509c0800'), (0xdb0d5, 'e896f1ffff')]:
        patch(call_rva, bytes.fromhex(expected),
              b'\xe8' + struct.pack('<i', code_rva + 5 * 6 - call_rva - 5),
              'Remove legacy metadata at final GPU-safe native resource destruction')

# Add8 to the exact native startup/common setter whitelist. The original
# success and error tails retain their native callbacks and state ownership.
msaa_validator_leaf = code_rva + len(code)
code.extend(antialiasing_validator_leaf())
patch(VALIDATOR_RVA, VALIDATOR_BEFORE,
      antialiasing_validator_patch(VALIDATOR_RVA, msaa_validator_leaf, VALIDATOR_ERROR_RVA),
      'Accept exactly1/2/4/8 in native common MSAA setter; preserve initialization and notifications')

mask_stub_rva = code_rva + len(code)
code.extend(bytes.fromhex(sample_mask_hook['stub_prefix']))
mask_jump_rva = code_rva + len(code)
mask_iat = iat + import_names.index(sample_mask_hook['export']) * 8
code.extend(b'\xff\x25' + struct.pack('<i', mask_iat - mask_jump_rva - 6))
mask_rva = int(sample_mask_hook['rva'], 16)
mask_patch = b'\xe8' + struct.pack('<i', mask_stub_rva - mask_rva - 5) + bytes.fromhex('898560010000')
mask_patch += b'\x90' * (sample_mask_hook['bytes'] - len(mask_patch))
patch(mask_rva, bytes.fromhex(sample_mask_hook['original']), mask_patch,
      'Map native coverage masks consistently for both cached and live forced-four sample PSOs')

if arguments.queue_diagnostics:
    copy_stub = code_rva + import_names.index('FM5ReadbackCopyTextureRegion') * 6
    for hook in copy_observer_plan(data, copy_stub, require_original=False):
        patch(hook['rva'], hook['original'], hook['replacement'],
              'Forward genuine seven-argument CopyTextureRegion and identify diagnostic-owned copies')
    queue_stub_rva = code_rva + len(code)
    # Private shadow space and fifth argument; RBP/R15 remain owned by the caller.
    code.extend(bytes.fromhex('4883ec38 4d8bcf 488b45f8 4889442420'.replace(' ', '')))
    call_rva = code_rva + len(code)
    wrapper_iat = iat + import_names.index('FM5DiagnosticExecuteCommandLists') * 8
    code.extend(b'\xff\x15' + struct.pack('<i', wrapper_iat - call_rva - 6))
    code.extend(bytes.fromhex('4883c438c3'))
    queue_stub_end = code_rva + len(code)
    unwind_rva = store(bytes.fromhex('0104010004620000'), 4)
    exceptions = pe.OPTIONAL_HEADER.DATA_DIRECTORY[3]
    native_runtime_functions = pe.get_data(exceptions.VirtualAddress, exceptions.Size)
    assert len(native_runtime_functions) % 12 == 0
    previous_begin = struct.unpack_from('<I', native_runtime_functions, len(native_runtime_functions) - 12)[0]
    assert previous_begin < queue_stub_rva
    code.extend(bytes(align(len(code), 4) - len(code)))
    runtime_functions_rva = code_rva + len(code)
    code.extend(native_runtime_functions + struct.pack('<III', queue_stub_rva, queue_stub_end, unwind_rva))
    struct.pack_into('<II', data, exceptions.get_file_offset(), runtime_functions_rva, len(native_runtime_functions) + 12)
    patch(0xf1f4e, bytes.fromhex('488b01ff5050'),
          b'\xe8' + struct.pack('<i', queue_stub_rva - 0xf1f4e - 5) + b'\x90',
          'Observe genuine command-list batch identity and fences without changing submission')

branding_report = apply_native_branding(pe, data, code, code_rva, patch)

section_header = pe.sections[-1].get_file_offset() + 40
assert section_header + 80 <= pe.sections[0].PointerToRawData
file_alignment = pe.OPTIONAL_HEADER.FileAlignment
for name, rva, blob, characteristics in [
    (b'.fm5dat', data_rva, payload, 0xc0000040),
    (b'.fm5code', code_rva, code, 0x60000020),
]:
    raw_offset = align(len(data), file_alignment)
    raw_size = align(len(blob), file_alignment)
    data.extend(bytes(raw_offset - len(data)))
    data.extend(blob)
    data.extend(bytes(raw_size - len(blob)))
    struct.pack_into('<8sIIIIIIHHI', data, section_header, name, len(blob), rva,
                     raw_size, raw_offset, 0, 0, 0, 0, characteristics)
    section_header += 40
struct.pack_into('<H', data, pe.FILE_HEADER.get_field_absolute_offset('NumberOfSections'),
                 pe.FILE_HEADER.NumberOfSections + 2)
struct.pack_into('<I', data, pe.OPTIONAL_HEADER.get_field_absolute_offset('SizeOfImage'),
                 align(code_rva + len(code), pe.OPTIONAL_HEADER.SectionAlignment))
struct.pack_into('<II', data, imports.get_file_offset(), data_rva, descriptor_offset + 40)
struct.pack_into('<I', data, pe.OPTIONAL_HEADER.get_field_absolute_offset('CheckSum'), 0)

verified = pefile.PE(data=bytes(data))
assert verified.DIRECTORY_ENTRY_IMPORT[-1].dll == b'ForzaMastro5Bridge.dll'
assert [entry.name for entry in verified.DIRECTORY_ENTRY_IMPORT[-1].imports] == [name.encode() for name in import_names]
assert all(not (tail_start <= entry.rva < tail_end and entry.type)
           for block in verified.DIRECTORY_ENTRY_BASERELOC for entry in block.entries)
OUT.mkdir(parents=True, exist_ok=True)
shutil.copytree(SOURCE / 'data', OUT / 'data', dirs_exist_ok=True,
                ignore=shutil.ignore_patterns('azimuth.ini', 'azimuth.log', 'azimuth.pipelines'))
with tempfile.TemporaryDirectory(prefix='scene-overrides-', dir=BUILD.resolve()) as scene_stage:
    stage_scene_overrides(GAME.resolve(), scene_stage, arguments.frame_rate)
    shutil.copytree(Path(scene_stage) / 'data/scene-overrides', OUT / 'data/scene-overrides', dirs_exist_ok=True)

target = OUT / 'ForzaMastro5.exe'
target.write_bytes(data)
(OUT / 'data/RaceOneEngine.ini').write_text(f'''[launcher]
game=7b35d917
game_folder={PLAY_GAME}
resolution=2160
aspect=16:9
antialiasing={arguments.antialiasing}
graphics={arguments.preset}
shadows={arguments.preset}
reflections={arguments.preset}
car_detail={arguments.preset}
particles={arguments.preset}
motion_blur={arguments.preset}
texture_filtering={arguments.preset}
audio=multichannel
frame_rate={arguments.frame_rate}
frame_pacing=real-time
vsync=on
display=fullscreen
hdr=off
language=it-IT
speech=system
player_name=Player 1

[title-7b35d917]
folder={PLAY_GAME}
name=Forza Motorsport 5
content=
''', encoding='utf-8')
report = {'source_sha256': EXPECTED, 'output_sha256': hashlib.sha256(data).hexdigest(),
          'native_branding': branding_report,
          'graphics_adapters': arguments.graphics_adapters,
          'd3d12_diagnostics': arguments.d3d12_diagnostics, 'bridge_imports': import_names,
          'gamepad_diagnostics': arguments.gamepad_diagnostics,
          'frame_timing': arguments.frame_timing,
          'queue_diagnostics': arguments.queue_diagnostics,
          'configured_preset': arguments.preset,
          'configured_antialiasing': arguments.antialiasing,
          'forced_msaa4_normalization': True,
          'icon': str(icon.relative_to(ROOT)),
          'configured_frame_rate': arguments.frame_rate,
          'path': str(target), 'changes': changes,
          'removed_fh2_relocations': removed_relocations,
          'new_sections': [{'name': s.Name.decode().strip('\0'), 'rva': hex(s.VirtualAddress),
                            'characteristics': hex(s.Characteristics)} for s in verified.sections[-2:]],
          'game_original_written': False, 'game_executed': False,
          'scope': 'Experimental FM5-only host. Native FM5 scheduler and GPU wait code retained.'}
(ROOT / 'analysis/pc-build/host-build.json').write_text(json.dumps(report, indent=2) + '\n')
print(f'Built experimental host: {target}')
