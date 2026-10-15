"""Verify the reviewed TLS recipe against the user's complete pinned FM5 EXE."""
from pathlib import Path
import hashlib
import re

GAME_SHA256 = '94dad21851d335e9a33231dc2a1de2ecd4b93eefdf5f6772085bf6886bf47db7'


def validate_tls_recipe(game_exe, recipe_header):
    import pefile
    from capstone import Cs, CS_ARCH_X86, CS_MODE_64
    raw = Path(game_exe).read_bytes()
    if hashlib.sha256(raw).hexdigest() != GAME_SHA256:
        raise ValueError('Unsupported FM5 executable for the reviewed TLS recipe.')
    pe = pefile.PE(data=raw)
    header = Path(recipe_header).read_text('utf-8')
    matches = re.findall(r'\{0x([0-9a-f]+)u,(\d+),(9|12),(\d+),\{([^}]+)\}\}', header)
    if len(matches) != 327 or '#define FM5_TLS_GUEST_ALL_SITES_CLASSIFIED 1u' not in header:
        raise ValueError('Incomplete reviewed TLS recipe.')
    sites = set()
    counts = {9: 0, 12: 0}
    for rva, destination, size, lookup, expected in matches:
        rva, destination, size, lookup = int(rva, 16), int(destination), int(size), int(lookup)
        value = bytes(int(x, 16) for x in expected.split(','))
        if rva in sites or destination == 4 or len(value) != size or pe.get_data(rva, size) != value:
            raise ValueError('TLS instruction guard mismatch or duplicate site.')
        if size == 9 and lookup != 255 or size == 12 and lookup not in (1, 2):
            raise ValueError('Unknown reviewed TLS recipe form.')
        counts[size] += 1
        sites.add(rva)
    # Require completeness against all GS:[58] vector loads in executable sections.
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.skipdata = True
    discovered = set()
    for section in pe.sections:
        if not section.Characteristics & 0x20000000:
            continue
        for ins in decoder.disasm(section.get_data(), section.VirtualAddress):
            if ins.mnemonic == 'mov' and 'gs:[0x58]' in ins.op_str:
                discovered.add(ins.address)
    if discovered != sites or counts != {9: 325, 12: 2}:
        raise ValueError('FM5 TLS recipe does not cover the complete executable.')
    return {'total_sites': len(sites), 'game_sha256': GAME_SHA256,
            'classification_counts': {'safe_vector0_only': counts[9],
                                      'exact_preserving_double_load': counts[12]}}
