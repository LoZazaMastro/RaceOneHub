"""File-only guards for the five native post-graphics-preparation observer calls."""
import hashlib
import json
from pathlib import Path
import struct


ROOT = Path(__file__).resolve().parents[1]
RECIPE = ROOT / "recipes/native-draw-observer.json"
ORIGINAL_SHA256 = "579ed8d8b0b16cbf7890380cb7b925c972b837631ede367eada55eae4afc39bc"


def rva_offset(data, rva, size):
    if len(data) < 0x100 or data[:2] != b"MZ":
        raise ValueError("Expected PE image.")
    nt = struct.unpack_from("<I", data, 0x3c)[0]
    if nt < 0x40 or nt + 24 + 112 > len(data) or data[nt:nt+4] != b"PE\0\0":
        raise ValueError("Invalid PE header.")
    machine, sections = struct.unpack_from("<HH", data, nt+4)
    opt_size = struct.unpack_from("<H", data, nt+20)[0]
    if machine != 0x8664 or not 1 <= sections <= 96 or opt_size < 112 or struct.unpack_from("<H", data, nt+24)[0] != 0x20b:
        raise ValueError("Expected x64 PE32+.")
    table = nt + 24 + opt_size
    if table + sections*40 > len(data):
        raise ValueError("Section headers out of bounds.")
    for index in range(sections):
        virtual_size, start, raw_size, raw_start = struct.unpack_from("<IIII", data, table+index*40+8)
        if start <= rva and size >= 0 and rva + size <= start + raw_size:
            offset = raw_start + rva - start
            if raw_start + raw_size > len(data) or offset + size > len(data):
                raise ValueError("Section payload out of bounds.")
            return offset
    raise ValueError("Guard RVA is not backed by file data.")


def validate(data, recipe=None, *, require_original=True):
    """No mutations: validate every site before an integrating builder patches any."""
    recipe = recipe or json.loads(RECIPE.read_text("utf-8"))
    if recipe.get("schema") != 1 or recipe.get("original_sha256") != ORIGINAL_SHA256:
        raise ValueError("Unknown observer recipe version.")
    if require_original and (len(data) != recipe["original_bytes"] or hashlib.sha256(data).hexdigest() != ORIGINAL_SHA256):
        raise ValueError("Original host SHA-256 mismatch.")
    callee = int(recipe["callee"]["rva"], 16)
    guard = bytes.fromhex(recipe["callee"]["guard"])
    offset = rva_offset(data, callee, len(guard))
    if data[offset:offset+len(guard)] != guard:
        raise ValueError("Native graphics preparation guard mismatch.")
    expected_sites = {0x12b8a7, 0x12b96e, 0x12ba47, 0x12bb0e, 0x12bbfe}
    sites = recipe["callsites"]
    if len(sites) != 5 or {int(x["rva"], 16) for x in sites} != expected_sites:
        raise ValueError("Incomplete or unexpected native draw callsite list.")
    for site in sites:
        address = int(site["rva"], 16)
        original = bytes.fromhex(site["original_call"])
        offset = rva_offset(data, address, 5)
        call = data[offset:offset+5]
        if len(original) != 5 or call != original or call[0] != 0xe8 or address + 5 + struct.unpack_from("<i", call, 1)[0] != callee:
            raise ValueError("Native preparation CALL mismatch.")
        for rva, field in [(site["preceding_rva"], "preceding_guard"),
                           (site["following_rva"], "following_guard"),
                           (site["caller_rva"], "caller_prologue_guard")]:
            expected = bytes.fromhex(site[field])
            at = rva_offset(data, int(rva, 16), len(expected))
            if data[at:at+len(expected)] != expected:
                raise ValueError("Native draw caller/context guard mismatch.")
    return {"passed": True, "callsites": 5, "callee": hex(callee),
            "original_sha_checked": require_original,
            "scope": "File-only guards; no runtime or observer behavior claim."}
