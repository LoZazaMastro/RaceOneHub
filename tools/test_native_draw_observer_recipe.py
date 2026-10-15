"""Actual original PE plus per-byte guard mutations; no executable loading."""
import json
from pathlib import Path
from native_draw_observer_recipe import ROOT, RECIPE, rva_offset, validate

data = (ROOT / "runtime/azimuth-0.9.9-original/azimuth.exe").read_bytes()
recipe = json.loads(RECIPE.read_text("utf-8"))
assert validate(data, recipe)["callsites"] == 5
positions = set()
for row in recipe["callsites"]:
    for rva, field in [(row["rva"], "original_call"), (row["preceding_rva"], "preceding_guard"),
                       (row["following_rva"], "following_guard"), (row["caller_rva"], "caller_prologue_guard")]:
        size = len(bytes.fromhex(row[field]))
        at = rva_offset(data, int(rva, 16), size)
        positions.update(range(at, at+size))
at = rva_offset(data, int(recipe["callee"]["rva"], 16), len(bytes.fromhex(recipe["callee"]["guard"])))
positions.update(range(at, at+len(bytes.fromhex(recipe["callee"]["guard"]))))
for position in positions:
    mutated = bytearray(data)
    mutated[position] ^= 1
    try:
        validate(mutated, recipe, require_original=False)
    except ValueError:
        pass
    else:
        raise AssertionError(f"Missed guard mutation at file offset{position:x}")
mutated = bytearray(data)
mutated[-1] ^= 1
try:
    validate(mutated, recipe)
except ValueError:
    pass
else:
    raise AssertionError("Original SHA mismatch was accepted.")
assert data == (ROOT / "runtime/azimuth-0.9.9-original/azimuth.exe").read_bytes()
report = {"passed": True, "callsites": 5, "byte_mutations_rejected": len(positions),
          "original_sha_mismatch_rejected": True, "original_unchanged": True,
          "scope": "Headless file-only PE guards; no game/DLL execution."}
(ROOT / "analysis/graphics-port/native-draw-observer-recipe-tests.json").write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
