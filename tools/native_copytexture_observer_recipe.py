"""Read-only guards and LEA-R10 byte plans for the two real native copy paths."""
import hashlib
import json
import struct
from native_draw_observer_recipe import ROOT, ORIGINAL_SHA256, rva_offset

RECIPE = ROOT / "recipes/native-copytexture-observer.json"


def replacement_plan(data, stub_rva, *, require_original=True):
    recipe = json.loads(RECIPE.read_text("utf-8"))
    if recipe.get("original_sha256") != ORIGINAL_SHA256 or recipe.get("total_arguments_including_this") != 7:
        raise ValueError("Unknown CopyTextureRegion recipe.")
    if require_original and (len(data) != recipe["original_bytes"] or hashlib.sha256(data).hexdigest() != ORIGINAL_SHA256):
        raise ValueError("Original host SHA-256 mismatch.")
    if type(stub_rva) is not int or not 0 <= stub_rva <= 0xffffffff:
        raise ValueError("Stub must be a valid PE RVA.")
    if len(recipe["hooks"]) != 2 or {int(x["rva"], 16) for x in recipe["hooks"]} != {0xee44a, 0xeeabc}:
        raise ValueError("Both direct and replay paths are required.")
    plan = []
    for hook in recipe["hooks"]:
        rva = int(hook["rva"], 16)
        expected = bytes.fromhex(hook["guard"])
        at = rva_offset(data, int(hook["guard_rva"], 16), len(expected))
        if data[at:at+len(expected)] != expected:
            raise ValueError("Native CopyTextureRegion path guard mismatch.")
        load = bytes.fromhex(hook["original_load"])
        at = rva_offset(data, rva, len(load))
        if load != bytes.fromhex("4c8b9080000000") or data[at:at+7] != load:
            raise ValueError("Expected exact MOV R10,[RAX+0x80].")
        displacement = stub_rva - (rva + 7)
        if not -(1 << 31) <= displacement < (1 << 31):
            raise ValueError("Observer stub lies outside RIP-relative range.")
        plan.append({"rva": rva, "file_offset": at, "original": load,
                     "replacement": b"\x4c\x8d\x15" + struct.pack("<i", displacement)})
    return plan
