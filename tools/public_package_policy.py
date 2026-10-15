"""Read-only audit of the INITIAL public builder archive, never a user's output.

This module does not assemble, publish, execute, download or modify anything.
Every released file needs an explicit relative path, length, SHA-256 and origin.
The current development manifest deliberately retains release blockers.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import stat


class PackagePolicyError(ValueError):
    pass


FORBIDDEN_NAMES = {
    "xg.dll", "xg_x.dll", "fm5-host-xg.dll", "forza_durango_release_final.exe",
    "raceone5.exe", "forzamastro5.exe", "azimuth.exe", "era.vbi",
    "raceoneengine.ini", "azimuth.ini", "builder-state.json", "settings.json",
    "build-profile.json", "launcher-capabilities.json",
}
FORBIDDEN_PARTS = {"game", "mount", "embeddedxvd", "saves", "titles", "logs",
                   "checkpoints", "save-backups", "analysis", "reference"}
FORBIDDEN_SUFFIXES = {".rar", ".xvd", ".vbi", ".dmp", ".pdb", ".obj", ".lib",
                      ".exp", ".log", ".jxr"}
# Exact externally supplied originals, denied even when renamed or moved.
EXTERNAL_HASHES = {
    "579ed8d8b0b16cbf7890380cb7b925c972b837631ede367eada55eae4afc39bc",
    "9e3a06d02784699c252bea271090b9e2a04ef2cb29b5b0647c6ce6e73b19db67",
    "dcea8f44cb4049695ed9f5890bc4e24a4c2e3e75d082333e3f388f509395a522",
    "9d198fcf130793cf0e2c742ff7e934bfd87c8ced0940b189413e52d6e874125a",
    "94dad21851d335e9a33231dc2a1de2ecd4b93eefdf5f6772085bf6886bf47db7",
    # Audited private bridge embeds original shader chunks and long code guards.
    "1c5ee4bb4bc8d95907bf0f3c28ebe22485ee314df10241a956cab13aca14f8fe",
}


def _relative(value):
    if not isinstance(value, str) or not value or "\\" in value or ":" in value:
        raise PackagePolicyError("An allowlist path must be relative POSIX text.")
    path = PurePosixPath(value)
    if path.is_absolute() or value != path.as_posix() or any(
            part in ("", ".", "..") or part.endswith((".", " ")) for part in value.split("/")):
        raise PackagePolicyError("An allowlist path must be normalized and stay inside the archive.")
    folded = [part.casefold() for part in path.parts]
    if folded[-1] in FORBIDDEN_NAMES or set(folded) & FORBIDDEN_PARTS or path.suffix.casefold() in FORBIDDEN_SUFFIXES:
        raise PackagePolicyError(f"Private/runtime input is not public builder content: {value}")
    return value


def _ordinary(path):
    info = path.lstat()
    if stat.S_ISLNK(info.st_mode) or getattr(info, "st_file_attributes", 0) & 0x400:
        raise PackagePolicyError("Links and reparse points are not allowed.")
    if not stat.S_ISREG(info.st_mode):
        raise PackagePolicyError("Expected an ordinary file.")
    return info


def _sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _root(path):
    path = Path(os.path.abspath(path))
    for ancestor in reversed((path, *path.parents)):
        info = ancestor.lstat()
        if stat.S_ISLNK(info.st_mode) or getattr(info, "st_file_attributes", 0) & 0x400:
            raise PackagePolicyError("The archive root cannot pass through a link or reparse point.")
    if not path.is_dir():
        raise PackagePolicyError("The archive root must be a directory.")
    return path


def audit_public_tree(root, manifest):
    """Exact-tree/hash audit. Passing containment is separate from release readiness."""
    root = _root(root)
    if manifest.get("schema") != 1 or manifest.get("scope") != "initial-public-builder":
        raise PackagePolicyError("This policy applies only to the initial public builder archive.")
    blockers = manifest.get("release_blockers")
    if not isinstance(blockers, list) or not all(isinstance(x, str) and x for x in blockers):
        raise PackagePolicyError("Release blockers must be an explicit list.")
    entries = manifest.get("files")
    if not isinstance(entries, list) or not entries:
        raise PackagePolicyError("An explicit nonempty file allowlist is required.")
    expected = {}
    for entry in entries:
        name = _relative(entry["path"])
        key = name.casefold()
        if key in expected:
            raise PackagePolicyError("Case-insensitive duplicate allowlist path.")
        if not re.fullmatch(r"[0-9a-f]{64}", entry.get("sha256", "")) or type(entry.get("bytes")) is not int or entry["bytes"] < 0:
            raise PackagePolicyError("Every file requires its exact size and SHA-256.")
        if not isinstance(entry.get("origin"), str) or not entry["origin"]:
            raise PackagePolicyError("Every file requires an explicit origin.")
        expected[key] = entry
    observed = {}

    def visit(directory):
        for path in sorted(directory.iterdir(), key=lambda x: x.name.casefold()):
            info = path.lstat()
            if stat.S_ISLNK(info.st_mode) or getattr(info, "st_file_attributes", 0) & 0x400:
                raise PackagePolicyError("Links and reparse points are not allowed.")
            name = path.relative_to(root).as_posix()
            if path.is_dir():
                if path.name.casefold() in FORBIDDEN_PARTS:
                    raise PackagePolicyError(f"Private folder in archive: {name}")
                if not any(key.startswith(name.casefold() + "/") for key in expected):
                    raise PackagePolicyError(f"Unlisted folder: {name}")
                visit(path)
                continue
            _ordinary(path)
            _relative(name)
            key = name.casefold()
            if key in observed:
                raise PackagePolicyError("Case-insensitive collision in archive.")
            if key not in expected:
                raise PackagePolicyError(f"Unlisted file: {name}")
            entry = expected[key]
            digest = _sha(path)
            if digest in EXTERNAL_HASHES:
                raise PackagePolicyError("An external game/runtime original is forbidden even under a different name.")
            if info.st_size != entry["bytes"] or digest != entry["sha256"]:
                raise PackagePolicyError(f"File differs from reviewed allowlist: {name}")
            observed[key] = digest

    visit(root)
    if set(expected) != set(observed):
        raise PackagePolicyError("Required allowlist file is missing.")
    return {"containment_passed": True, "release_ready": not blockers,
            "files_verified": len(observed), "release_blockers": list(blockers),
            "scope": "Initial builder files only; no gameplay or distribution-rights approval implied."}


def validate_external_input(path, expected):
    """Validate an explicitly selected original WITHOUT loading or executing it."""
    path = Path(os.path.abspath(path))
    _root(path.parent)
    info = _ordinary(path)
    if info.st_size != expected["bytes"] or _sha(path) != expected["sha256"]:
        raise PackagePolicyError("Unsupported external input (exact size/SHA-256 mismatch).")
    return {"validated": True, "id": expected["id"], "bytes": info.st_size,
            "sha256": expected["sha256"], "loaded": False, "copied": False}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    args = parser.parse_args()
    try:
        result = audit_public_tree(args.root, json.loads(args.manifest.read_text("utf-8")))
        print(json.dumps(result, indent=2))
        raise SystemExit(0 if result["release_ready"] else 2)
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(json.dumps({"containment_passed": False, "release_ready": False, "error": str(error)}))
        raise SystemExit(1)
