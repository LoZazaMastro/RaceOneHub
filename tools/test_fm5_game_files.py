"""Small independent fixtures. Never copy or alter the actual game files."""
import hashlib
import json
import os
from pathlib import Path
import stat
import struct
import tempfile
import unittest
from unittest.mock import patch

import fm5_game_files as game


class GameFilesTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="fm5-game-files-")
        self.addCleanup(self.temporary.cleanup)
        self.base = Path(self.temporary.name)
        self.source = self.base / "original"
        (self.source / "Mount/media/renderscenarios").mkdir(parents=True)
        (self.source / "EmbeddedXvd/Windows").mkdir(parents=True)
        self.manifest = self.source / "Mount/AppxManifest.xml"
        self.manifest.write_text('<Package xmlns="http://schemas.microsoft.com/appx/2010/manifest">'
                                 '<Identity Name="Forza" Version="1.0.0.20" ProcessorArchitecture="x64"/>'
                                 '<Applications><Application Executable="Forza_Durango_Release_Final.exe"/>'
                                 '</Applications></Package>', encoding="utf-8")
        (self.source / "Mount" / game.EXE_NAME).write_bytes(b"fixture executable; never executed")
        (self.source / "era.vbi").write_bytes(b"fixture vbi")
        (self.source / "EmbeddedXvd/Windows/fixture.bin").write_bytes(b"embedded fixture")
        for name in game.SCENE_NAMES:
            (self.source / game.SCENE_DIRECTORY / name).write_bytes(
                b'<?xml version="1.0"?>\r\n<RenderScenario>\r\n<FrameRate value="30"/>\r\n'
                b'<Other value="30"/><!-- intact -->\r\n</RenderScenario>')
        (self.source / game.SCENE_DIRECTORY / "Race.xml").write_bytes(
            b'<RenderScenario><FrameRate value="60"/></RenderScenario>')
        digest = hashlib.sha256((self.source / "Mount" / game.EXE_NAME).read_bytes()).hexdigest()
        self.addCleanup(patch.stopall)
        patch.object(game, "EXPECTED_EXE_SHA256", digest).start()

    def snapshot(self, root):
        return {path.relative_to(root).as_posix(): (path.read_bytes(), path.stat().st_mtime_ns)
                for path in root.rglob("*") if path.is_file()}

    def test_source_root_and_mount_inventory_are_equivalent(self):
        before = self.snapshot(self.source)
        report = game.validate_game_source(self.source)
        self.assertEqual(report, game.validate_game_source(self.source / 'Mount'))
        self.assertEqual(self.snapshot(self.source), before)

    def test_only_six_overrides_and_no_game_copy_for_all_fps(self):
        before = self.snapshot(self.source)
        for fps in (30, 60, 120):
            stage = self.base / f'stage-{fps}'
            events = []
            result = game.stage_scene_overrides(self.source, stage, fps, events.append)
            self.assertEqual(result['game_root'], str(self.source))
            self.assertFalse(result['game_files_copied'])
            self.assertEqual(result['copied_files'], 0)
            self.assertEqual(result['override_files'], 6)
            self.assertFalse((stage / 'game').exists())
            self.assertEqual(len(list(stage.rglob('*.xml'))), 6)
            for name in game.SCENE_NAMES:
                wanted = (self.source / game.SCENE_DIRECTORY / name).read_bytes().replace(game._OLD_FRAME_RATE, game._NEW_FRAME_RATE)
                self.assertEqual((stage / 'data/scene-overrides' / name).read_bytes(), wanted)
            self.assertEqual(events[-1]['bytes_copied'], result['override_bytes'])
            self.assertEqual(self.snapshot(self.source), before)

    def test_invalid_fps_no_output(self):
        for fps in (0, True, 59, '60'):
            with self.assertRaises(game.GameFilesError):
                game.stage_scene_overrides(self.source, self.base / 'stage', fps)
        self.assertFalse((self.base / 'stage').exists())

    def test_nested_output_refused(self):
        for stage in (self.source, self.source / 'nested', self.base):
            with self.assertRaisesRegex(game.GameFilesError, 'nested'):
                game.stage_scene_overrides(self.source, stage, 60)

    def test_existing_overrides_refused(self):
        stage=self.base / 'stage'
        game.stage_scene_overrides(self.source, stage, 60)
        before=self.snapshot(stage)
        with self.assertRaisesRegex(game.GameFilesError, 'already exists'):
            game.stage_scene_overrides(self.source, stage, 60)
        self.assertEqual(self.snapshot(stage), before)

    def test_modified_source_scene_refused(self):
        scene=self.source / game.SCENE_DIRECTORY / 'FrontEnd.xml'
        scene.write_bytes(scene.read_bytes().replace(game._OLD_FRAME_RATE, game._NEW_FRAME_RATE))
        with self.assertRaisesRegex(game.GameFilesError, 'recipe'):
            game.stage_scene_overrides(self.source, self.base / 'stage', 60)

    def test_concurrent_source_change_aborts(self):
        stage=self.base / 'stage'
        def change(event):
            (self.source / 'changed.txt').write_bytes(b'fixture')
        with self.assertRaisesRegex(game.GameFilesError, 'source tree changed'):
            game.stage_scene_overrides(self.source, stage, 60, change)
        self.assertFalse((stage / 'data/scene-overrides').exists())
        self.assertFalse(list(stage.rglob('scene-overrides.preparing-*')))

    def test_cancel_aborts_only_own_overrides(self):
        stage=self.base / 'stage'
        stage.mkdir();(stage / 'keep.txt').write_bytes(b'keep')
        def cancel(event): raise RuntimeError('cancelled')
        with self.assertRaisesRegex(RuntimeError, 'cancelled'):
            game.stage_scene_overrides(self.source, stage, 60, cancel)
        self.assertEqual((stage / 'keep.txt').read_bytes(), b'keep')
        self.assertFalse(list(stage.rglob('scene-overrides.preparing-*')))

    def test_readonly_originals_not_modified(self):
        scene=self.source / game.SCENE_DIRECTORY / 'FrontEnd.xml'
        scene.chmod(stat.S_IREAD)
        try:
            before=self.snapshot(self.source)
            game.stage_scene_overrides(self.source, self.base / 'stage', 120)
            self.assertEqual(self.snapshot(self.source), before)
            self.assertFalse(scene.stat().st_mode & stat.S_IWRITE)
        finally: scene.chmod(stat.S_IREAD | stat.S_IWRITE)

    def test_original_executable_hash_required(self):
        (self.source / 'Mount' / game.EXE_NAME).write_bytes(b'wrong executable')
        with self.assertRaisesRegex(game.GameFilesError, 'SHA-256 mismatch'):
            game.validate_game_source(self.source)

    def test_reparse_points_are_rejected_in_tree_and_ancestors(self):
        real_lstat = Path.lstat
        marker = self.source / "EmbeddedXvd"
        class ReparseInfo:
            st_file_attributes = 0x400
            st_mode = stat.S_IFDIR
        def fake_lstat(path):
            return ReparseInfo() if path == marker else real_lstat(path)
        with patch.object(Path, "lstat", fake_lstat):
            with self.assertRaisesRegex(game.GameFilesError, "Reparse point"):
                game.validate_game_source(marker / "Windows")
        real_scan = os.scandir
        class EntryProxy:
            def __init__(self, entry):
                self.entry, self.path, self.name = entry, entry.path, entry.name
            def stat(self, follow_symlinks=False):
                return ReparseInfo() if Path(self.path) == marker else self.entry.stat(follow_symlinks=follow_symlinks)
        class ScanProxy:
            def __init__(self, path): self.entries = real_scan(path)
            def __enter__(self): return iter(EntryProxy(entry) for entry in self.entries)
            def __exit__(self, *args): self.entries.close()
        with patch.object(game.os, "scandir", ScanProxy):
            with self.assertRaisesRegex(game.GameFilesError, "Reparse point"):
                game.validate_game_source(self.source)

    def test_case_collision_is_refused_even_on_case_insensitive_windows(self):
        real_scan = os.scandir
        class EntryProxy:
            def __init__(self, entry):
                self.entry, self.path, self.name = entry, entry.path.upper(), entry.name.upper()
            def stat(self, follow_symlinks=False): return self.entry.stat(follow_symlinks=follow_symlinks)
        class ScanProxy:
            def __init__(self, path): self.path, self.entries = path, real_scan(path)
            def __enter__(self):
                entries = list(self.entries)
                # Add a second name differing only by case at the root.
                return iter(entries + [EntryProxy(next(entry for entry in entries if entry.name == "era.vbi"))]
                            if Path(self.path) == self_source else entries)
            def __exit__(self, *args): self.entries.close()
        self_source = self.source
        with patch.object(game.os, "scandir", ScanProxy):
            with self.assertRaisesRegex(game.GameFilesError, "collision"):
                game.validate_game_source(self.source)

    def test_shortcut_requires_completed_package_and_valid_name(self):
        target = self.base / "launcher.exe"
        target.write_bytes(b"not executable; COM only")
        with self.assertRaisesRegex(game.GameFilesError, "committed"):
            game.create_desktop_shortcut(target)
        with self.assertRaisesRegex(game.GameFilesError, "name"):
            game.create_desktop_shortcut(target, package_committed=True, name="../escape")

    @unittest.skipUnless(os.name == "nt", "Windows COM shortcut fixture")
    def test_real_com_shortcut_in_fixture_desktop_only_and_no_overwrite(self):
        desktop = self.base / "fixture desktop"
        desktop.mkdir()
        target = self.base / "launcher ' $literal` ; é.exe"
        target.write_bytes(b"never execute this file")
        result = game.create_desktop_shortcut(target, package_committed=True, desktop=desktop,
                                              name="Forza fixture é", icon_path=target)
        link = desktop / "Forza fixture é.lnk"
        self.assertEqual(Path(result), link)
        contents = link.read_bytes()
        self.assertEqual(struct.unpack_from("<I", contents)[0], 76)
        self.assertIn(target.name.encode("utf-16le"), contents)
        with self.assertRaisesRegex(game.GameFilesError, "already exists"):
            game.create_desktop_shortcut(target, package_committed=True, desktop=desktop,
                                         name="Forza fixture é")
        self.assertEqual(link.read_bytes(), contents)
        self.assertFalse(list(desktop.glob(".fm5-*")))
        default_link = game.create_desktop_shortcut(target, package_committed=True, desktop=desktop)
        self.assertEqual(Path(default_link).name, "RaceOne5.lnk")
        self.assertIn("RaceOne5".encode("utf-16le"), Path(default_link).read_bytes())


if __name__ == "__main__":
    unittest.main(verbosity=2)
