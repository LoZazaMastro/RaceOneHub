"""Headless file-only validation; no native executable or D3D object loaded."""
import json
import struct
from native_copytexture_observer_recipe import ROOT, RECIPE, replacement_plan
from native_draw_observer_recipe import rva_offset

data = (ROOT / "runtime/azimuth-0.9.9-original/azimuth.exe").read_bytes()
recipe = json.loads(RECIPE.read_text("utf-8"))
stub = 0x3e8000
plan = replacement_plan(data, stub)
assert len(plan) == 2
patched = bytearray(data)
changed = set()
for patch in plan:
    at, value = patch["file_offset"], patch["replacement"]
    assert len(value) == 7 and value[:3] == b"\x4c\x8d\x15"
    assert patch["rva"] + 7 + struct.unpack_from("<i", value, 3)[0] == stub
    patched[at:at+7] = value
    changed.update(range(at, at+7))
assert all(a == b for index, (a, b) in enumerate(zip(data, patched)) if index not in changed)
positions = set()
for hook in recipe["hooks"]:
    size = len(bytes.fromhex(hook["guard"]))
    at = rva_offset(data, int(hook["guard_rva"], 16), size)
    positions.update(range(at, at+size))
for position in positions:
    mutation = bytearray(data)
    mutation[position] ^= 1
    try:
        replacement_plan(mutation, stub, require_original=False)
    except ValueError:
        pass
    else:
        raise AssertionError(f"Native path mutation accepted at file{position:x}")
for invalid in [-1, 0x100000000, True, "0x3e8000", 0xffffffff]:
    try:
        replacement_plan(data, invalid)
    except ValueError:
        pass
    else:
        raise AssertionError("Invalid/out-of-range stub accepted.")
assert data == (ROOT / "runtime/azimuth-0.9.9-original/azimuth.exe").read_bytes()
report = {"passed": True, "native_paths": 2, "arguments_including_this": 7,
          "byte_mutations_rejected": len(positions), "changes_only_two_7byte_loads": True,
          "original_unchanged": True, "scope": "File-only guards/encoding; no game/DLL/GPU execution."}
(ROOT / "analysis/graphics-port/native-copytexture-observer-recipe-tests.json").write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
