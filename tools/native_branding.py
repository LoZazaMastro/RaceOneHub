"""Guarded presentation-only native branding; original binary stays immutable.

Export names, runtime type names and save-format markers remain ABI/data contracts.
User-facing literals are relocated into the builder's existing read-only code
section. No longer string is written into an original literal's allocation.
"""
from pathlib import Path
import ctypes as ct
from ctypes import wintypes as wt
import json
import struct

RECIPE = Path(__file__).with_name('native_branding_literals.json')


def apply_native_branding(pe, data, code, code_rva, patch):
    recipe = json.loads(RECIPE.read_text(encoding='utf-8'))
    # Complete preflight before changing data or appending the new string pool.
    for item in recipe['literals']:
        rva = int(item['rva'], 16)
        before = (item['old'] + '\0').encode(item['encoding'])
        offset = pe.get_offset_from_rva(rva)
        if data[offset:offset + len(before)] != before:
            raise ValueError(f'Native branding literal guard failed at {rva:#x}')
        for ref in item['references']:
            site = int(ref['rva'], 16)
            before = bytes.fromhex(ref['bytes'])
            off = pe.get_offset_from_rva(site)
            if data[off:off + len(before)] != before or len(before) != 7 or before[1] != 0x8d:
                raise ValueError(f'Native branding reference guard failed at {site:#x}')
            if site + 7 + struct.unpack_from('<i', before, 3)[0] != rva:
                raise ValueError(f'Native branding reference target failed at {site:#x}')
    for item in recipe['lengths'] + recipe['in_place']:
        rva = int(item['rva'], 16)
        before = bytes.fromhex(item['before'])
        offset = pe.get_offset_from_rva(rva)
        if data[offset:offset + len(before)] != before:
            raise ValueError(f'Native branding length/copy guard failed at {rva:#x}')

    report = {'engine_name': 'RaceOneEngine', 'strings': [], 'lengths': recipe['lengths'],
              'preserved_contracts': ['TLS DLL/export names', 'RTTI/WinRT type identities',
                                      'save commit format identifiers', 'copyright/credits']}
    for item in recipe['literals']:
        if item['encoding'] == 'utf-16le' and (code_rva + len(code)) & 1:
            code.append(0)
        target = code_rva + len(code)
        code.extend((item['new'] + '\0').encode(item['encoding']))
        for ref in item['references']:
            site = int(ref['rva'], 16)
            before = bytes.fromhex(ref['bytes'])
            after = before[:3] + struct.pack('<i', target - site - len(before))
            patch(site, before, after, 'RaceOneEngine presentation string: ' + item['new'])
        report['strings'].append({'original_rva': item['rva'], 'target_rva': hex(target),
                                  'text': item['new'], 'encoding': item['encoding'],
                                  'references': len(item['references'])})
    for item in recipe['lengths'] + recipe['in_place']:
        patch(int(item['rva'], 16), bytes.fromhex(item['before']), bytes.fromhex(item['after']),
              'RaceOneEngine explicit string length or bounded inline-copy wording')
    return report


def _block(key, value=b'', kind=1, children=(), value_length=None):
    content = bytearray(bytes(6) + (key + '\0').encode('utf-16le'))
    content.extend(bytes((-len(content)) & 3))
    content.extend(value)
    for child in children:
        content.extend(bytes((-len(content)) & 3))
        content.extend(child)
    length = len(value) // 2 if kind == 1 else len(value)
    struct.pack_into('<HHH', content, 0, len(content),
                     length if value_length is None else value_length, kind)
    return bytes(content)


def embed_native_branding_resources(executable, original_filename='RaceOne5.exe'):
    """Update derived PE version/manifest resources without running the image."""
    import pefile
    original = pefile.PE(str(executable))
    version = None
    values = {}
    version_language = None
    manifest_items = []
    for entry in original.DIRECTORY_ENTRY_RESOURCE.entries:
        if entry.id not in (16, 24):
            continue
        for name in entry.directory.entries:
            if name.id != 1:
                raise ValueError('Unrecognized native branding resource name')
            for language in name.directory.entries:
                descriptor = language.data.struct
                blob = original.get_data(descriptor.OffsetToData, descriptor.Size)
                if entry.id == 16:
                    if version is not None:
                        raise ValueError('Unexpected multiple native version resources')
                    version = blob
                    version_language = language.id
                else:
                    manifest = blob.decode('utf-8')
                    if 'name="Azimuth.Desktop"' not in manifest:
                        raise ValueError('Native assembly branding guard failed')
                    manifest_items.append((language.id, manifest.replace(
                        'name="Azimuth.Desktop"', 'name="RaceOneEngine.Desktop"').encode('utf-8')))
    for info in original.FileInfo:
        for section in info:
            if hasattr(section, 'StringTable'):
                if values:
                    raise ValueError('Unexpected native version string tables')
                values = {k.decode('ascii'): v.decode('utf-8') for k, v in section.StringTable[0].entries.items()}
    if not version or values.get('ProductName') != 'Azimuth' or not manifest_items:
        raise ValueError('Native version resource branding guard failed')
    fixed_offset = version.find(struct.pack('<I', 0xfeef04bd))
    if fixed_offset < 0 or len(version) < fixed_offset + 52:
        raise ValueError('Native fixed version resource guard failed')
    fixed = version[fixed_offset:fixed_offset + 52]
    # Preserve the upstream copyright, company and true runtime version.
    if original_filename != 'RaceOne5.exe':
        raise ValueError('Unsupported Race One game executable identity')
    values.update(FileDescription='RaceOneEngine', InternalName='RaceOneEngine',
                  ProductName='RaceOneEngine', OriginalFilename=original_filename)
    strings = [_block(key, (value + '\0').encode('utf-16le')) for key, value in values.items()]
    table = _block('040904b0', children=strings)
    string_info = _block('StringFileInfo', children=[table])
    translation = _block('Translation', struct.pack('<HH', 0x409, 1200), kind=0)
    var_info = _block('VarFileInfo', children=[translation])
    replacement = _block('VS_VERSION_INFO', fixed, kind=0, children=[string_info, var_info])
    original.close()

    kernel = ct.WinDLL('kernel32', use_last_error=True)
    kernel.BeginUpdateResourceW.argtypes = [wt.LPCWSTR, wt.BOOL]
    kernel.BeginUpdateResourceW.restype = wt.HANDLE
    kernel.UpdateResourceW.argtypes = [wt.HANDLE, ct.c_void_p, ct.c_void_p, wt.WORD, ct.c_void_p, wt.DWORD]
    kernel.UpdateResourceW.restype = wt.BOOL
    kernel.EndUpdateResourceW.argtypes = [wt.HANDLE, wt.BOOL]
    kernel.EndUpdateResourceW.restype = wt.BOOL
    transaction = kernel.BeginUpdateResourceW(str(executable), False)
    if not transaction:
        raise ct.WinError(ct.get_last_error())
    try:
        for kind, language, blob in [(16, version_language, replacement)] + [
                (24, lang, text) for lang, text in manifest_items]:
            buffer = ct.create_string_buffer(blob)
            if not kernel.UpdateResourceW(transaction, kind, 1, language, buffer, len(blob)):
                raise ct.WinError(ct.get_last_error())
        if not kernel.EndUpdateResourceW(transaction, False):
            raise ct.WinError(ct.get_last_error())
        transaction = None
    finally:
        if transaction:
            kernel.EndUpdateResourceW(transaction, True)
