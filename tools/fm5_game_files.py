"""Validate user-owned FM5 files and prepare six small local scene overrides.
The selected game directory is read directly and never copied or modified.
"""
from __future__ import annotations

import ctypes
from dataclasses import dataclass
import hashlib
import os
from pathlib import Path
import shutil
import stat
import subprocess
import uuid
import xml.etree.ElementTree as ET
from raceone_games import game_definition

EXPECTED_EXE_SHA256 = "94dad21851d335e9a33231dc2a1de2ecd4b93eefdf5f6772085bf6886bf47db7"
EXE_NAME = "forza_durango_release_final.exe"
SCENE_DIRECTORY = "Mount/media/renderscenarios"
SCENE_NAMES = ("FrontEnd.xml", "MiniAutoVista.xml", "PhotoMode.xml",
               "PostRace.xml", "PreRace_321.xml", "Replay.xml")
_OLD_FRAME_RATE = b'<FrameRate value="30"/>'
_NEW_FRAME_RATE = b'<FrameRate value="60"/>'


class GameFilesError(ValueError):
    """An actionable validation/import refusal, safe to show in the builder UI."""


@dataclass(frozen=True)
class _Entry:
    relative: str
    directory: bool
    size: int
    mtime_ns: int
    device: int
    inode: int


def _reparse(info):
    return stat.S_ISLNK(info.st_mode) or bool(getattr(info, "st_file_attributes", 0) & 0x400)


def _plain_path(path):
    """Check ancestors before resolve(), which would otherwise hide junctions."""
    path = Path(os.path.abspath(os.fspath(path)))
    for part in reversed((path, *path.parents)):
        try:
            info = part.lstat()
        except FileNotFoundError:
            continue
        if _reparse(info):
            raise GameFilesError(f"Reparse point or symbolic link is not allowed: {part}")
    return path.resolve()


def _scan(root):
    entries = []
    names = set()

    def visit(folder):
        with os.scandir(folder) as children:
            children = sorted(children, key=lambda child: child.name.casefold())
        for child in children:
            relative = Path(child.path).relative_to(root).as_posix()
            key = relative.casefold()
            if key in names:
                raise GameFilesError(f"Case-insensitive path collision: {relative}")
            names.add(key)
            info = child.stat(follow_symlinks=False)
            if _reparse(info):
                raise GameFilesError(f"Reparse point or symbolic link is not allowed: {relative}")
            # Windows DirEntry.stat may leave st_ino/st_dev at zero. Use the
            # same full lstat representation for the initial/final snapshots.
            info = Path(child.path).lstat()
            if _reparse(info):
                raise GameFilesError(f"Reparse point or symbolic link is not allowed: {relative}")
            directory = stat.S_ISDIR(info.st_mode)
            if not directory and not stat.S_ISREG(info.st_mode):
                raise GameFilesError(f"Only ordinary files and folders are allowed: {relative}")
            entries.append(_Entry(relative, directory, 0 if directory else info.st_size,
                                  info.st_mtime_ns, info.st_dev, info.st_ino))
            if directory:
                visit(child.path)

    visit(root)
    return entries


def _sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _bounded_xml(path):
    if path.stat().st_size > 1024 * 1024:
        raise GameFilesError(f"Unexpectedly large XML file: {path.name}")
    data = path.read_bytes()
    try:
        document = ET.fromstring(data)
    except ET.ParseError as error:
        raise GameFilesError(f"Invalid XML in {path.name}: {error}") from error
    return data, document


def _inspect(selection, game_id='fm5'):
    game = game_definition(game_id)
    root = _plain_path(selection)
    if root.name.casefold() == "mount":
        root = root.parent
    if not root.is_dir():
        raise GameFilesError(f"Select the extracted {game.name} folder, or its Mount folder.")
    # Fail quickly for an accidental drive/home-folder selection rather than
    # recursively inventory an unrelated tree before noticing the manifest.
    if not (root / "Mount/AppxManifest.xml").is_file():
        raise GameFilesError("Required FM5 file is missing: Mount/AppxManifest.xml. Select the extracted game root or Mount.")
    entries = _scan(root)
    paths = {entry.relative.casefold(): entry for entry in entries}

    def required(relative, directory=False):
        entry = paths.get(relative.casefold())
        if entry is None or entry.directory != directory:
            raise GameFilesError(f"Required FM5 {'folder' if directory else 'file'} is missing: {relative}")
        if not directory and entry.size == 0:
            raise GameFilesError(f"Required FM5 file is empty: {relative}")
        return root / entry.relative

    _, manifest = _bounded_xml(required("Mount/AppxManifest.xml"))
    identities = [element for element in manifest.iter() if element.tag.split("}")[-1] == "Identity"]
    if len(identities) != 1 or any(identities[0].get(key) != value for key, value in {
            "Name": game.package, "Version": game.version, "ProcessorArchitecture": "x64"}.items()):
        raise GameFilesError(f"Unsupported game: expected {game.name}, version {game.version}, architecture x64.")
    applications = [element for element in manifest.iter() if element.tag.split("}")[-1] == "Application"]
    if not any(element.get("Executable", "").casefold() == EXE_NAME for element in applications):
        raise GameFilesError("The manifest does not reference the supported FM5 executable.")
    executable = required("Mount/" + EXE_NAME)
    executable_hash = _sha256(executable)
    if executable_hash != (EXPECTED_EXE_SHA256 if game_id == 'fm5' else game.sha256):
        raise GameFilesError(f"The {game.name} executable does not match the supported original {game.version} file (SHA-256 mismatch).")
    required("era.vbi")
    required("EmbeddedXvd", directory=True)
    scenes = []
    for name in game.scenes:
        path = required(SCENE_DIRECTORY + "/" + name)
        data, document = _bounded_xml(path)
        frame_nodes = document.findall("FrameRate")
        if len(frame_nodes) != 1 or frame_nodes[0].get("value") != "30" or data.count(_OLD_FRAME_RATE) != 1:
            raise GameFilesError(f"The original 30 FPS scene recipe does not match {name}.")
        scenes.append((path.relative_to(root).as_posix(), data))
    report = {"source_root": str(root), "game_root": str(root), "identity": dict(identities[0].attrib),
              "executable_sha256": executable_hash,
              "files": sum(not entry.directory for entry in entries),
              "directories": sum(entry.directory for entry in entries),
              "source_bytes": sum(entry.size for entry in entries),
              "scene_paths": [relative for relative, _ in scenes]}
    return root, entries, scenes, report


def validate_game_source(selection, game_id='fm5'):
    """Read-only validation/inventory; accepts the extracted root or its Mount."""
    try:
        return _inspect(selection, game_id)[3]
    except OSError as error:
        raise GameFilesError(f"Cannot read the selected game files: {error}") from error


def _nested(a, b):
    # normcase provides Windows semantics, including drive-letter casing.
    a, b = Path(os.path.normcase(str(a))), Path(os.path.normcase(str(b)))
    return a == b or a in b.parents or b in a.parents


def _commit_directory(temporary, target):
    if os.name == "nt":
        # MoveFileW (no REPLACE_EXISTING) also rejects a destination created by
        # another process after our first check; both paths are on one volume.
        move = ctypes.WinDLL("kernel32", use_last_error=True).MoveFileW
        move.argtypes = (ctypes.c_wchar_p, ctypes.c_wchar_p)
        move.restype = ctypes.c_int
        if not move(str(temporary), str(target)):
            raise ctypes.WinError(ctypes.get_last_error())
    else:
        # The shipping builder is Windows-only. Refuse unsupported commit
        # semantics rather than let POSIX rename replace an existing folder.
        raise GameFilesError("Committing a game package requires Windows.")


def stage_scene_overrides(selection, staging_root, frame_rate, progress=None, game_id='fm5'):
    """Create only data/scene-overrides; keep the original game as the VFS root.

    Both 60 and 120 FPS use the known scene mode 60. Always prepare the six
    overrides so the native pre-game can switch between 30/60/120 later. At 30
    the runtime reads the original files and bypasses these overrides.
    """
    if type(frame_rate) is not int or frame_rate not in (30, 60, 120):
        raise GameFilesError("Frame rate must be 30, 60 or 120.")
    temporary = None
    try:
        root, entries, scenes, report = _inspect(selection, game_id)
        staging = _plain_path(staging_root)
        if _nested(root, staging):
            raise GameFilesError("Game source and output folders must be separate, not nested.")
        target = staging / "data/scene-overrides"
        if target.exists() or target.is_symlink():
            raise GameFilesError(f"Output already exists: {target}")
        target.parent.mkdir(parents=True, exist_ok=True)
        temporary = target.parent / ("scene-overrides.preparing-" + uuid.uuid4().hex)
        temporary.mkdir()
        changes = []
        total = sum(len(data) for _, data in scenes)
        written = 0
        for relative, original in scenes:
            if (root / relative).read_bytes() != original:
                raise GameFilesError("The source scene changed during preparation.")
            modified = original.replace(_OLD_FRAME_RATE, _NEW_FRAME_RATE)
            if ET.fromstring(modified).find("FrameRate").get("value") != "60":
                raise GameFilesError(f"The scene profile could not be applied: {relative}")
            (temporary / Path(relative).name).write_bytes(modified)
            changes.append({"path": relative, "before_sha256": hashlib.sha256(original).hexdigest(),
                            "after_sha256": hashlib.sha256(modified).hexdigest()})
            written += len(modified)
            if progress:
                progress({"phase": "preparing_scene_overrides", "files_copied": 0,
                          "total_files": 0, "bytes_copied": written, "total_bytes": total})
        if _scan(root) != entries:
            raise GameFilesError("The source tree changed during preparation.")
        _commit_directory(temporary, target)
        temporary = None
        return {**report, "ready": True, "frame_rate": frame_rate,
                "native_scene_frame_rate": "original" if frame_rate == 30 else 60,
                "changes": changes, "copied_files": 0, "game_files_copied": False,
                "override_files": len(changes), "override_bytes": total,
                "hardlinks_used": False, "original_files_modified": False}
    except OSError as error:
        raise GameFilesError(f"Scene overrides could not be prepared: {error}") from error
    finally:
        if temporary is not None and temporary.is_dir():
            shutil.rmtree(temporary)


def create_desktop_shortcut(target_exe, *, package_committed=False, name="RaceOne5", desktop=None, icon_path=None):
    """Optional final step. Caller must have committed its complete package.

    No shortcut is created by stage_scene_overrides. Existing .lnk files are refused.
    desktop is useful for a caller-selected Desktop; None uses Windows' actual
    Desktop location (including folder redirection/OneDrive).
    """
    if package_committed is not True:
        raise GameFilesError("A Desktop shortcut can be created only after the complete package is committed.")
    target = _plain_path(target_exe)
    if not target.is_file() or target.suffix.casefold() != ".exe":
        raise GameFilesError("The committed launcher executable is missing.")
    if not name or name in (".", "..") or any(ord(character) < 32 or character in '<>:"/\\|?*' for character in name) or name.endswith((" ", ".")):
        raise GameFilesError("Invalid Desktop shortcut name.")
    if os.name != "nt":
        raise GameFilesError("Desktop shortcuts require Windows.")
    script = Path(__file__).with_name("fm5_create_shortcut.ps1")
    system_root = Path(os.environ.get("SystemRoot", r"C:\Windows"))
    command = [str(system_root / "System32/WindowsPowerShell/v1.0/powershell.exe"),
               "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-File", str(script),
               "-TargetPath", str(target), "-LinkName", name, "-WorkingDirectory", str(target.parent)]
    if desktop is not None:
        command += ["-DesktopPath", str(_plain_path(desktop))]
    if icon_path is not None:
        icon = _plain_path(icon_path)
        if not icon.is_file():
            raise GameFilesError("The shortcut icon file is missing.")
        command += ["-IconPath", str(icon)]
    try:
        result = subprocess.run(command, capture_output=True, text=True, encoding="utf-8", errors="replace",
                                creationflags=subprocess.CREATE_NO_WINDOW, timeout=30, check=False)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise GameFilesError(f"Desktop shortcut was not created: {error}") from error
    if result.returncode:
        raise GameFilesError("Desktop shortcut was not created: " + (result.stderr.strip() or "Windows COM operation failed."))
    return result.stdout.strip()
