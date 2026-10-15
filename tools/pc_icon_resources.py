"""Embed the local FM5 icon in the derived host, preserving every code RVA."""
from pathlib import Path
import ctypes as ct
from ctypes import wintypes as wt
import struct


def embed_icon_in_place(executable: Path, icon: Path):
    """Replace icon resources without moving sections or appended native code.

    Unlike UpdateResource, this uses only the existing icon storage. Refuse an
    unfamiliar resource layout or insufficient capacity before writing anything.
    """
    import pefile
    content = icon.read_bytes()
    reserved, kind, count = struct.unpack_from('<HHH', content)
    if reserved or kind != 1 or not 0 < count < 32:
        raise ValueError('Invalid icon directory')
    before = executable.read_bytes()
    pe = pefile.PE(data=before)
    resources = {}
    for entry in pe.DIRECTORY_ENTRY_RESOURCE.entries:
        if entry.id in (3, 14):
            resources[entry.id] = [language.data.struct
                for name in entry.directory.entries for language in name.directory.entries]
    images = resources.get(3, [])
    groups = resources.get(14, [])
    if len(groups) != 1 or not images or groups[0].Size < 20:
        raise ValueError(f'Unsupported existing icon resource layout: images={len(images)}, groups={len(groups)}, group_sizes={[d.Size for d in groups]}, requested={count}')
    source_count = count
    count = min(count, len(images), (groups[0].Size - 6) // 14)
    icon_type = next(t for t in pe.DIRECTORY_ENTRY_RESOURCE.entries if t.id == 3)
    ids = [n.id for n in icon_type.directory.entries]
    if len(ids) != len(images) or any(i is None for i in ids):
        raise ValueError('Unsupported multilingual or named icon resources')
    intervals = sorted((pe.get_offset_from_rva(d.OffsetToData), d.Size, d.OffsetToData)
                       for d in images)
    for (offset, size, _), (next_offset, _, _) in zip(intervals, intervals[1:]):
        if offset + size > next_offset:
            raise ValueError('Overlapping existing icon allocations')
    pending = []
    for index in range(source_count):
        fields = struct.unpack_from('<BBBBHHII', content, 6 + index * 16)
        size, start = fields[-2:]
        if not size or start < 6 + source_count * 16 or start + size > len(content):
            raise ValueError('Invalid icon image extent')
        pending.append((index, fields, content[start:start + size]))
    # Older native hosts have only one icon slot. Keep the largest source frame;
    # Windows scales it for small taskbar sizes without expanding PE resources.
    pending = sorted(pending, key=lambda row: (row[1][0] or 256) * (row[1][1] or 256))[-count:]
    pending = [(index, fields, blob) for index, (_, fields, blob) in enumerate(pending)]
    bins = [list(row) for row in intervals]
    placements = {}
    for index, fields, blob in sorted(pending, key=lambda row: len(row[2]), reverse=True):
        candidates = [b for b in bins if b[1] >= len(blob)]
        if not candidates:
            raise ValueError('New icon exceeds existing resource capacity')
        target = min(candidates, key=lambda b: b[1])
        placements[index] = (target[0], target[2])
        target[0] += len(blob)
        target[1] -= len(blob)
        target[2] += len(blob)
    data = bytearray(before)
    for offset, size, _ in intervals:
        data[offset:offset+size] = bytes(size)
    group = bytearray(struct.pack('<HHH', 0, 1, count))
    for index, fields, blob in pending:
        offset, rva = placements[index]
        data[offset:offset+len(blob)] = blob
        struct.pack_into('<II', data, images[index].get_file_offset(), rva, len(blob))
        group.extend(struct.pack('<BBBBHHIH', *fields[:6], len(blob), ids[index]))
    group_offset = pe.get_offset_from_rva(groups[0].OffsetToData)
    data[group_offset:group_offset+groups[0].Size] = group + bytes(groups[0].Size-len(group))
    struct.pack_into('<I', data, groups[0].get_field_absolute_offset('Size'), len(group))
    resource_section = pe.get_section_by_rva(groups[0].OffsetToData)
    for section in pe.sections:
        if section is resource_section:
            continue
        offset, size = section.PointerToRawData, section.SizeOfRawData
        if data[offset:offset+size] != before[offset:offset+size]:
            raise ValueError('Icon edit would alter a non-resource section')
    if len(data) != len(before):
        raise ValueError('Icon edit changed the executable layout')
    pe.close()
    executable.write_bytes(data)
    return {'frames': count, 'section_layout_preserved': True, 'code_unchanged': True}


def embed_icon(executable: Path, icon: Path):
    content = icon.read_bytes()
    reserved, kind, count = struct.unpack_from('<HHH', content)
    if reserved or kind != 1 or not 0 < count < 32:
        raise ValueError('Invalid icon directory')
    kernel = ct.WinDLL('kernel32', use_last_error=True)
    kernel.BeginUpdateResourceW.argtypes = [wt.LPCWSTR, wt.BOOL]
    kernel.BeginUpdateResourceW.restype = wt.HANDLE
    kernel.UpdateResourceW.argtypes = [wt.HANDLE, ct.c_void_p, ct.c_void_p, wt.WORD, ct.c_void_p, wt.DWORD]
    kernel.EndUpdateResourceW.argtypes = [wt.HANDLE, wt.BOOL]
    transaction = kernel.BeginUpdateResourceW(str(executable), False)
    if not transaction:
        raise ct.WinError(ct.get_last_error())
    group = bytearray(struct.pack('<HHH', 0, 1, count))
    blobs = []
    try:
        # Enumerate the original resources through pefile before updating.
        import pefile
        original = pefile.PE(str(executable))
        for resource_type in original.DIRECTORY_ENTRY_RESOURCE.entries:
            if resource_type.id not in [3, 14]:
                continue
            for name in resource_type.directory.entries:
                if name.id is None:
                    raise ValueError('Named icon resources need explicit handling')
                for language in name.directory.entries:
                    if not kernel.UpdateResourceW(transaction, resource_type.id, name.id, language.id, None, 0):
                        raise ct.WinError(ct.get_last_error())
        original.close()
        for index in range(count):
            offset = 6 + index * 16
            width, height, palette, padding, planes, bits, size, start = struct.unpack_from('<BBBBHHII', content, offset)
            if start + size > len(content):
                raise ValueError('Invalid icon image extent')
            blob = ct.create_string_buffer(content[start:start + size]); blobs.append(blob)
            if not kernel.UpdateResourceW(transaction, 3, index + 1, 0, blob, size):
                raise ct.WinError(ct.get_last_error())
            group.extend(struct.pack('<BBBBHHIH', width, height, palette, padding, planes, bits, size, index + 1))
        group_blob = ct.create_string_buffer(bytes(group))
        if not kernel.UpdateResourceW(transaction, 14, 1, 0, group_blob, len(group)):
            raise ct.WinError(ct.get_last_error())
        if not kernel.EndUpdateResourceW(transaction, False):
            raise ct.WinError(ct.get_last_error())
        transaction = None
    finally:
        if transaction:
            kernel.EndUpdateResourceW(transaction, True)
