from pathlib import Path
import configparser
import hashlib
import json
import os
import tempfile
import unittest
from unittest import mock
import pc_launcher_backend as backend
import fm5_game_files as game_files


class LauncherSafetyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        runtime = self.root / 'runtime/pc'; (runtime / 'data').mkdir(parents=True)
        for file in ['ForzaMastro5.exe', 'ForzaMastro5Bridge.dll']:
            (runtime / file).write_bytes(b'NOT EXECUTABLE - TEST FIXTURE')
        self.ini = runtime / 'data/RaceOneEngine.ini'
        self.ini.write_text('[launcher]\ngame=7b35d917\ngame_folder=C:\\A Game\\Mount\nresolution=2160\ngraphics=default\nplayer_name=Andrea\n\n[title-7b35d917]\nfolder=C:\\A Game\\Mount\ncontent=Storm Island\n')
        self.original = self.ini.read_bytes()
        (runtime / 'launcher-capabilities.json').write_text(json.dumps({
            'ready': True, 'validated_resolutions': [1080,1440,2160,2880,3240,3780,4320],
            'validated_presets': ['low','default','high','ultra'],
            'native_pre_game': True,
            'diagnostic_environment': {'FM5_FRAME_TIMING':'1'}}))
        self.running = mock.patch.object(backend, 'running_pids', return_value=[])
        self.running.start(); self.addCleanup(self.running.stop)

    def test_no_apply_while_running(self):
        with mock.patch.object(backend, 'running_pids', return_value=[123]):
            with self.assertRaisesRegex(RuntimeError, 'Close'):
                backend.apply_and_launch(self.root, backend.DEFAULTS.copy(), True)
        self.assertEqual(self.ini.read_bytes(), self.original)

    def test_bad_requests_never_write(self):
        requests = [dict(backend.DEFAULTS, resolution=4096), dict(backend.DEFAULTS, frame_rate=True),
                    dict(backend.DEFAULTS, vsync=1), dict(backend.DEFAULTS, command='cmd.exe'),
                    {k:v for k,v in backend.DEFAULTS.items() if k != 'graphics'}]
        for request in requests:
            with self.subTest(request=request):
                with self.assertRaises(ValueError):
                    backend.apply_and_launch(self.root, request, True)
                self.assertEqual(self.ini.read_bytes(), self.original)

    def test_apply_preserves_game_identity_and_user_data(self):
        result = backend.apply_and_launch(self.root, backend.DEFAULTS.copy(), False)
        ini = backend.load_ini(self.root)
        self.assertTrue(result['ok'])
        before = configparser.ConfigParser(); before.read_string(self.original.decode())
        self.assertEqual(ini['launcher']['game_folder'], before['launcher']['game_folder'])
        self.assertEqual(ini['launcher']['player_name'], 'Andrea')
        self.assertEqual(ini['title-7b35d917']['content'], 'Storm Island')
        self.assertEqual(ini['launcher']['antialiasing'], 'msaax4')
        self.assertEqual(ini['launcher']['vsync'], 'on')
        self.assertTrue(all(ini['launcher'][g] == 'ultra' for g in backend.GROUPS))
        self.assertEqual(list(self.ini.parent.glob('*.tmp')), [])

    def test_retired_eight_sample_profile_migrates_to_four(self):
        legacy = dict(backend.DEFAULTS, antialiasing='msaax8')
        result = backend.apply_and_launch(self.root, legacy, False)
        self.assertTrue(result['ok'])
        self.assertEqual(backend.load_ini(self.root)['launcher']['antialiasing'], 'msaax4')
        self.assertEqual(legacy['antialiasing'], 'msaax8')
        self.assertEqual(backend.SUPPORTED['antialiasing'], ['off', 'msaax2', 'msaax4'])

    def test_all_nine_values_and_antialiasing_roundtrip_for_supported_game(self):
        for game_id in ('fm5',):
            game = backend.game_definition(game_id)
            runtime = self.root / game.runtime
            (runtime / 'data').mkdir(parents=True, exist_ok=True)
            (runtime / 'data/RaceOneEngine.ini').write_text('[launcher]\nantialiasing=msaax8\n')
            state = backend.capabilities(self.root, game_id=game_id)
            self.assertEqual(state['current']['antialiasing'], 'msaax4')
            self.assertEqual(state['supported']['antialiasing'], ['off', 'msaax2', 'msaax4'])
            for aa in ('off', 'msaax2', 'msaax4', 'msaax8'):
                with self.subTest(game=game_id, antialiasing=aa):
                    request = dict(backend.DEFAULTS, antialiasing=aa, frame_rate=60, resolution=1440,
                        graphics='high', display='windowed', vsync=False, language='en-US', audio='stereo', show_pre_game=False)
                    result = backend.validate_settings(json.loads(json.dumps(request)))
                    self.assertEqual(len(result), 9)
                    self.assertIs(type(result['vsync']), bool)
                    self.assertIs(type(result['show_pre_game']), bool)
                    self.assertIs(type(result['frame_rate']), int)
                    self.assertIs(type(result['resolution']), int)
                    self.assertEqual(result, dict(request, antialiasing='msaax4' if aa=='msaax8' else aa))
                    profile = configparser.ConfigParser()
                    profile.read_string(backend.configured_ini(self.root, result, 'fixture-game', game_id))
                    self.assertEqual(profile['launcher']['antialiasing'], result['antialiasing'])
                    self.assertEqual(profile['launcher']['game'], game.title_id)
                    self.assertEqual(profile['launcher']['language'], 'en-US')
                    self.assertEqual(profile['launcher']['audio'], 'stereo')
                    self.assertEqual(profile['launcher']['vsync'], 'off')
                    self.assertNotIn('show_pre_game', profile['launcher'])

    def test_large_resolutions_roundtrip_without_starting_game(self):
        for height in (3240, 3780, 4320):
            with self.subTest(height=height):
                with mock.patch.object(backend.subprocess, 'Popen') as launch:
                    result = backend.apply_and_launch(self.root, dict(backend.DEFAULTS, resolution=height), False)
                self.assertTrue(result['ok'])
                self.assertEqual(int(backend.load_ini(self.root)['launcher']['resolution']), height)
                launch.assert_not_called()

    def test_unvalidated_profile_refused(self):
        path = self.root / 'runtime/pc/launcher-capabilities.json'
        path.write_text(json.dumps({'ready': True, 'validated_resolutions':[2160], 'validated_presets':['default']}))
        with self.assertRaisesRegex(RuntimeError, 'preset'):
            backend.apply_and_launch(self.root, backend.DEFAULTS.copy(), False)
        self.assertEqual(self.ini.read_bytes(), self.original)

    def test_launch_failure_restores_profile(self):
        with mock.patch.object(backend.subprocess, 'Popen', side_effect=OSError('fixture launch failure')):
            with self.assertRaises(OSError):
                backend.apply_and_launch(self.root, backend.DEFAULTS.copy(), True)
        self.assertEqual(self.ini.read_bytes(), self.original)

    def test_detached_launch_contract_and_probe_isolation(self):
        with mock.patch.dict(os.environ, {'FM5_APU_NATIVE_PROBE':'1', 'FM5_OTHER_TEST':'1'}):
            with mock.patch.object(backend.subprocess, 'Popen', return_value=mock.Mock(pid=123)) as launch:
                result = backend.apply_and_launch(self.root, backend.DEFAULTS.copy(), True)
        args, kw = launch.call_args
        self.assertEqual(args[0], [str(self.root/'runtime/pc/ForzaMastro5.exe'), '--skip-launcher'])
        self.assertEqual(kw['cwd'], self.root/'runtime/pc')
        self.assertEqual({k:v for k,v in kw['env'].items() if k.startswith('FM5_')}, {'FM5_FRAME_TIMING':'1'})
        self.assertEqual(kw['creationflags'], backend.subprocess.DETACHED_PROCESS | backend.subprocess.CREATE_NEW_PROCESS_GROUP)
        self.assertEqual(result['pid'], 123)
        self.assertFalse(json.loads(Path(result['session_file']).read_text())['automatic_stop'])


class BuilderSafetyTests(LauncherSafetyTests):
    def setUp(self):
        super().setUp()
        self.game = self.root / 'game-data'
        (self.game / 'Mount').mkdir(parents=True)
        (self.game / 'Mount/AppxManifest.xml').write_text(
            '<Package xmlns="http://schemas.microsoft.com/appx/2010/manifest">'
            '<Identity Name="Forza" Version="1.0.0.20" ProcessorArchitecture="x64"/>'
            '<Applications><Application Executable="Forza_Durango_Release_Final.exe"/>'
            '</Applications></Package>', encoding='utf-8')
        executable = self.game / 'Mount' / game_files.EXE_NAME
        executable.write_bytes(b'ORIGINAL GAME FIXTURE - NEVER EXECUTE')
        digest = hashlib.sha256(executable.read_bytes()).hexdigest()
        digest_patch = mock.patch.object(game_files, 'EXPECTED_EXE_SHA256', digest)
        digest_patch.start(); self.addCleanup(digest_patch.stop)
        (self.game / 'era.vbi').write_bytes(b'VBI fixture')
        system = self.game / 'EmbeddedXvd/Windows/System32'
        system.mkdir(parents=True)
        for name in ['xg.dll', 'xg_x.dll']:
            (system / name).write_bytes(b'GAME SDK FIXTURE: ' + name.encode())
        scenes = self.game / game_files.SCENE_DIRECTORY
        scenes.mkdir(parents=True)
        for name in game_files.SCENE_NAMES:
            (scenes / name).write_bytes(b'<Scenario><FrameRate value="30"/><Other value="7"/></Scenario>')
        (scenes / 'Race.xml').write_bytes(b'<Scenario><FrameRate value="60"/></Scenario>')
        self.ini.write_text(self.ini.read_text().replace(r'C:\A Game\Mount', str(self.game)))
        self.original = self.ini.read_bytes()
        for path in ['runtime/builder/ForzaMastro5.exe', 'runtime/pc/data/bin/codec.dll',
                     *['runtime/pc/data/bin/' + name for name in ['avcodec-62.dll','avutil-60.dll','azimuth-tls.dll','msvcp110.dll','msvcr110.dll']],
                     'runtime/pc/data/licenses.txt', 'runtime/xg-host/xg.dll',
                     'pregame/dist/ForzaMastro5.PreGame.exe']:
            file = self.root / path; file.parent.mkdir(parents=True, exist_ok=True)
            file.write_bytes(b'BUILD FIXTURE')
        texts = self.game / 'Mount/media/stringtables'; texts.mkdir(parents=True)
        (texts / 'IT.zip').write_bytes(b'Presence-only test fixture')
        (texts / 'CZ.zip').write_bytes(b'Presence-only test fixture')
        self.output = self.root / 'exports/RaceOne5/RaceOne5.exe'
        # Packaging safety fixtures deliberately contain non-PE bytes. The icon
        # algorithm has separate real-PE tests; here verify its integration/order.
        icon_patch = mock.patch.object(backend, 'embed_icon_in_place', return_value={'code_unchanged': True})
        self.icon_embed = icon_patch.start(); self.addCleanup(icon_patch.stop)


    def source_snapshot(self):
        return {p.relative_to(self.game).as_posix(): (p.read_bytes(), p.stat().st_mtime_ns)
                for p in self.game.rglob('*') if p.is_file()}

    def test_build_creates_bundle_without_launch_or_source_mutation(self):
        original_game = self.source_snapshot()
        with mock.patch.object(backend.subprocess, 'Popen') as popen:
            result = backend.build_game(self.root, backend.DEFAULTS.copy(), self.output, self.game)
        popen.assert_not_called()
        self.assertTrue(result['ok'])
        self.assertEqual(self.output.read_bytes(), b'NOT EXECUTABLE - TEST FIXTURE')
        self.icon_embed.assert_called_once()
        icon_target, icon_source = self.icon_embed.call_args.args
        self.assertEqual(icon_target.name, 'RaceOne5.exe')
        self.assertEqual(icon_source, self.root / 'launcher/ForzaMastro.Launcher/Assets/ForzaMotorsport5.ico')
        self.assertTrue((self.output.parent / 'ForzaMastro5Bridge.dll').is_file())
        self.assertTrue((self.output.parent / 'xg-host/xg.dll').is_file())
        self.assertEqual((self.output.parent / 'xg-host/xg.dll').read_bytes(), b'GAME SDK FIXTURE: xg.dll')
        self.assertEqual((self.output.parent / 'xg-host/xg_x.dll').read_bytes(), b'GAME SDK FIXTURE: xg_x.dll')
        self.assertEqual(self.ini.read_bytes(), self.original)
        self.assertEqual(self.source_snapshot(), original_game)
        profile = configparser.ConfigParser()
        profile.read(self.output.parent / 'data/RaceOneEngine.ini')
        self.assertEqual(profile['launcher']['graphics'], 'ultra')
        self.assertEqual(profile['launcher']['game_folder'], str(self.game))
        self.assertEqual(profile['title-7b35d917']['folder'], str(self.game))
        self.assertNotIn('show_pre_game', profile['launcher'])
        self.assertFalse((self.output.parent / 'data/bin/codec.dll').exists())
        self.assertEqual({p.name for p in (self.output.parent / 'data/bin').iterdir()}, {'avcodec-62.dll','avutil-60.dll','azimuth-tls.dll','msvcp110.dll','msvcr110.dll'})
        self.assertFalse((self.output.parent / 'runtime').exists())
        self.assertFalse((self.output.parent / 'ForzaMastro5.PreGame.exe').exists())
        self.assertIn('show_pre_game=1', (self.output.parent / 'forzamastro.ini').read_text())
        self.assertFalse((self.output.parent / 'data/titles').exists())
        self.assertFalse(result['game_files_copied'])
        report = json.loads((self.output.parent / 'build-profile.json').read_text())
        self.assertFalse(report['game_files_copied'])
        self.assertFalse((self.output.parent / 'game').exists())
        self.assertEqual(report['game_reference']['copied_files'], 0)
        self.assertFalse(report['game_reference']['hardlinks_used'])
        self.assertEqual(len(report['game_reference']['changes']), 6)
        self.assertFalse(list(self.output.parent.parent.glob('.forzamastro-build-*')))

    def test_icon_failure_rolls_back_uncommitted_bundle(self):
        with mock.patch.object(backend, 'embed_icon_in_place', side_effect=ValueError('fixture unsupported icon layout')):
            with self.assertRaisesRegex(ValueError, 'icon layout'):
                backend.build_game(self.root, backend.DEFAULTS.copy(), self.output, self.game)
        self.assertFalse(self.output.exists())
        self.assertEqual(self.ini.read_bytes(), self.original)
        self.assertFalse(list(self.output.parent.parent.glob('.forzamastro-build-*')))

    def test_direct_start_build_omits_pre_game_application(self):
        backend.build_game(self.root, dict(backend.DEFAULTS, show_pre_game=False), self.output, self.game)
        self.assertFalse((self.output.parent / 'ForzaMastro5.PreGame.exe').exists())
        self.assertIn('show_pre_game=0', (self.output.parent / 'forzamastro.ini').read_text())

    def test_language_controls_game_with_native_speech_following(self):
        backend.build_game(self.root, dict(backend.DEFAULTS, language='cs-CZ'), self.output, self.game / 'Mount')
        ini = configparser.ConfigParser(); ini.read(self.output.parent / 'data/RaceOneEngine.ini')
        self.assertEqual(ini['launcher']['language'], 'cs-CZ')
        self.assertEqual(ini['launcher']['speech'], 'system')
        languages = json.loads((self.output.parent / 'data/forzamastro-languages.json').read_text())
        self.assertEqual(set(languages), {'it-IT', 'cs-CZ'})

    def test_missing_localization_is_refused(self):
        with self.assertRaisesRegex(RuntimeError, 'not present'):
            backend.build_game(self.root, dict(backend.DEFAULTS, language='fr-FR'), self.output, self.game)
        self.assertFalse(self.output.exists())

    def test_previous_eight_field_profiles_migrate(self):
        old = {k:v for k,v in backend.DEFAULTS.items() if k not in ['speech', 'show_pre_game']}
        upgraded = backend.validate_settings(old)
        self.assertNotIn('speech', upgraded)
        legacy = backend.validate_settings(dict(backend.DEFAULTS, language='system', speech='system'))
        self.assertEqual(legacy['language'], 'it-IT')
        self.assertNotIn('speech', legacy)
        self.assertIs(upgraded['show_pre_game'], True)

    def test_nonempty_output_is_never_overwritten(self):
        self.output.parent.mkdir(parents=True)
        keep = self.output.parent / 'keep.txt'; keep.write_text('original')
        with self.assertRaisesRegex(ValueError, 'empty'):
            backend.build_game(self.root, backend.DEFAULTS.copy(), self.output, self.game)
        self.assertEqual(keep.read_text(), 'original')

    def test_build_failure_removes_only_private_staging(self):
        self.output.parent.mkdir(parents=True)
        with mock.patch.object(backend.shutil, 'copy2', side_effect=OSError('fixture')):
            with self.assertRaises(OSError):
                backend.build_game(self.root, backend.DEFAULTS.copy(), self.output, self.game)
        self.assertEqual(list(self.output.parent.iterdir()), [])
        self.assertEqual(list(self.output.parent.parent.glob('.forzamastro-build-*')), [])
        self.assertEqual(self.ini.read_bytes(), self.original)

    def test_project_component_output_refused(self):
        with self.assertRaisesRegex(ValueError, 'separate'):
            backend.build_game(self.root, backend.DEFAULTS.copy(),
                               self.root / 'runtime/new/RaceOne5.exe', self.game)

    def test_wrong_executable_name_refused(self):
        with self.assertRaises(ValueError):
            backend.build_game(self.root, backend.DEFAULTS.copy(),
                               self.output.with_name('UnsupportedGame.exe'), self.game)

    def test_game_folder_is_required_even_if_ini_contains_a_path(self):
        with self.assertRaisesRegex(ValueError, 'Choose the folder'):
            backend.build_game(self.root, backend.DEFAULTS.copy(), self.output)
        self.assertFalse(self.output.parent.exists())

    def test_original_executable_hash_is_required(self):
        (self.game / 'Mount' / game_files.EXE_NAME).write_bytes(b'UNSUPPORTED EXE')
        with self.assertRaisesRegex(game_files.GameFilesError, 'SHA-256 mismatch'):
            backend.build_game(self.root, backend.DEFAULTS.copy(), self.output, self.game)
        self.assertFalse(self.output.parent.exists())

    def test_all_three_fps_requests_keep_original_root_and_prepare_overrides(self):
        original_game = self.source_snapshot()
        for fps in (30, 60, 120):
            with self.subTest(fps=fps):
                output = self.output.parent.parent / f'fps-{fps}/RaceOne5.exe'
                result = backend.build_game(self.root, dict(backend.DEFAULTS, frame_rate=fps), output, self.game)
                self.assertTrue(result['ok'])
                profile = configparser.ConfigParser(); profile.read(output.parent / 'data/RaceOneEngine.ini')
                self.assertEqual(profile['launcher']['frame_rate'], str(fps))
                for name in game_files.SCENE_NAMES:
                    data = (output.parent / 'data/scene-overrides' / name).read_bytes()
                    self.assertIn(b'value="60"', data)
                    self.assertNotIn(b'value="120"', data)
                self.assertEqual(self.source_snapshot(), original_game)

    def test_shortcut_is_optional_and_called_only_after_commit(self):
        def create_shortcut(target, **options):
            self.assertEqual(target, self.output)
            self.assertEqual(options, {'package_committed': True})
            self.assertTrue(target.is_file())
            self.assertTrue((target.parent / 'build-profile.json').is_file())
            self.assertFalse((target.parent / 'game').exists())
            self.assertTrue((self.game / 'era.vbi').is_file())
            self.assertFalse(list(target.parent.parent.glob('.forzamastro-build-*')))
            return 'fixture Desktop/RaceOne5.lnk'
        with mock.patch.object(backend, 'create_desktop_shortcut', side_effect=create_shortcut) as shortcut:
            result = backend.build_game(self.root, backend.DEFAULTS.copy(), self.output, self.game, True)
        shortcut.assert_called_once()
        self.assertEqual(result['desktop_shortcut'], 'fixture Desktop/RaceOne5.lnk')
        self.assertEqual(result['warnings'], [])
        json.dumps(result)

    def test_shortcut_failure_preserves_successfully_committed_build(self):
        with mock.patch.object(backend, 'create_desktop_shortcut', side_effect=OSError('fixture COM failure')):
            result = backend.build_game(self.root, backend.DEFAULTS.copy(), self.output, self.game, True)
        self.assertTrue(result['ok'])
        self.assertTrue(self.output.is_file())
        self.assertIsNone(result['desktop_shortcut'])
        self.assertIn('fixture COM failure', result['warnings'][0])

    def test_build_failure_never_creates_shortcut(self):
        with mock.patch.object(backend, 'create_desktop_shortcut') as shortcut:
            with mock.patch.object(backend.shutil, 'copy2', side_effect=OSError('fixture')):
                with self.assertRaises(OSError):
                    backend.build_game(self.root, backend.DEFAULTS.copy(), self.output, self.game, True)
        shortcut.assert_not_called()
        self.assertFalse(self.output.exists())

    def test_output_nested_in_game_is_refused(self):
        with self.assertRaisesRegex(ValueError, 'separate'):
            backend.build_game(self.root, backend.DEFAULTS.copy(), self.game / 'output/RaceOne5.exe', self.game)
        self.assertFalse((self.game / 'output').exists())

    def test_unvalidated_builder_runtime_is_refused(self):
        path = self.root / 'runtime/pc/launcher-capabilities.json'
        manifest = json.loads(path.read_text()); manifest['native_pre_game'] = False
        path.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(RuntimeError, 'not ready'):
            backend.build_game(self.root, backend.DEFAULTS.copy(), self.output, self.game)
        self.assertFalse(self.output.exists())


if __name__ == '__main__':
    unittest.main()
