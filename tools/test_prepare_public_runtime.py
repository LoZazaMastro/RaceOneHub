"""Preparation safety/closure tests; no host or game execution."""
from pathlib import Path
import hashlib
import json
import tempfile
import unittest
from unittest import mock
import prepare_public_runtime as preparation


class RuntimePreparationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def test_checked_input_requires_exact_size_and_hash(self):
        path = self.root/'user input.dll'
        path.write_bytes(b'only a nonexecutable fixture')
        expected = {'bytes': path.stat().st_size, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
        self.assertEqual(preparation.checked_input(path, expected), path)
        for changes in [{'bytes': 0}, {'sha256': '0'*64}]:
            with self.assertRaisesRegex(ValueError, 'Unsupported input'):
                preparation.checked_input(path, dict(expected, **changes))
        self.assertEqual(path.read_bytes(), b'only a nonexecutable fixture')

    def test_missing_input_never_creates_output(self):
        output = self.root/'runtime'
        with self.assertRaises(ValueError):
            preparation.prepare(self.root/'absent', self.root/'game', self.root/'xg.dll', output)
        self.assertFalse(output.exists())
        self.assertEqual(list(self.root.iterdir()), [])

    def test_authorized_background_decodes_with_native_service(self):
        from fm5_background import validate_image
        a = self.root/'a.png'
        a.write_bytes((preparation.ROOT/'pregame/Assets/fm5-background-clean.png').read_bytes())
        decoded = validate_image(a)
        self.assertTrue(decoded['windows_wic_decode_passed'])
        self.assertGreater(decoded['width'],1000)
        a.write_bytes(a.read_bytes()[:40])
        with self.assertRaises(OSError):
            validate_image(a)

    def test_copy_source_excludes_private_include_and_data(self):
        source = self.root/'source'
        (source/'src/allocation-port').mkdir(parents=True)
        (source/'tools').mkdir()
        (source/'recipes').mkdir()
        (source/'launcher/ForzaMastro.Launcher/Assets').mkdir(parents=True)
        (source/'src/authored.c').write_text('int authored;')
        (source/'src/allocation-port/fm5_language_guards.inc').write_text('private generated data')
        (source/'tools/script.py').write_text('pass')
        (source/'recipes/guard.json').write_text('{}')
        (source/'src/game.exe').write_bytes(b'NOT EXECUTABLE')
        (source/'launcher/ForzaMastro.Launcher/Assets/ForzaMotorsport5.ico').write_bytes(b'authored icon fixture')
        target = self.root/'stage'
        with mock.patch.object(preparation, 'ROOT', source):
            preparation.copy_authored_source(target)
        self.assertTrue((target/'src/authored.c').is_file())
        self.assertFalse((target/'src/allocation-port/fm5_language_guards.inc').exists())
        self.assertFalse((target/'src/game.exe').exists())
        self.assertEqual(len([p for p in target.rglob('*') if p.is_file()]), 4)

    def test_subprocess_failure_is_reported_and_logged(self):
        result = mock.Mock(returncode=1, stdout='compiler output', stderr='precise failure')
        with mock.patch.object(preparation.subprocess, 'run', return_value=result):
            with self.assertRaisesRegex(RuntimeError, 'precise failure'):
                preparation.run(['compiler', 'argument with spaces'], self.root, 'fixture')
        self.assertEqual((self.root/'analysis/preparation/fixture.log').read_text(), 'compiler outputprecise failure')

    def test_subprocess_does_not_use_shell_or_flatten_arguments(self):
        result = mock.Mock(returncode=0, stdout='ok', stderr='')
        with mock.patch.object(preparation.subprocess, 'run', return_value=result) as runner:
            preparation.run(['compiler', 'argument with spaces'], self.root, 'fixture')
        args, kwargs = runner.call_args
        self.assertEqual(args[0], ['compiler', 'argument with spaces'])
        self.assertNotIn('shell', kwargs)

    def test_catalog_contains_only_pins_not_payloads(self):
        catalog = json.loads((preparation.ROOT/'recipes/runtime-inputs.json').read_text())
        self.assertEqual(set(catalog), {'schema','engine_version','engine_files','modern_xg','game_sdk','scope'})
        self.assertEqual(len(catalog['engine_files']), 7)
        for entry in [*catalog['engine_files'].values(), catalog['modern_xg'], *catalog['game_sdk'].values()]:
            self.assertEqual(set(entry), {'bytes', 'sha256'})
            self.assertRegex(entry['sha256'], r'^[0-9a-f]{64}$')
        self.assertNotIn('data/bin/azimuth.pipelines', catalog['engine_files'])

    def fixture_inputs(self):
        project = self.root/'project'
        (project/'recipes').mkdir(parents=True)
        game = self.root/'game'
        sdk = game/'EmbeddedXvd/Windows/System32'
        sdk.mkdir(parents=True)
        engine = self.root/'engine'
        engine.mkdir()
        modern = self.root/'modern/input.dll'
        modern.parent.mkdir()
        paths = [engine/'azimuth.exe', modern, sdk/'xg.dll', sdk/'xg_x.dll']
        for path in paths:
            path.write_bytes(b'NONEXECUTABLE PREPARATION FIXTURE')
        spec = {'bytes': paths[0].stat().st_size, 'sha256': preparation.digest(paths[0])}
        catalog = {'engine_files': {'azimuth.exe': spec}, 'modern_xg': spec,
                   'game_sdk': {'xg.dll': spec, 'xg_x.dll': spec}}
        (project/'recipes/runtime-inputs.json').write_text(json.dumps(catalog))
        image=project/'pregame/Assets/fm5-background-clean.png'
        image.parent.mkdir(parents=True)
        image.write_bytes((preparation.ROOT/'pregame/Assets/fm5-background-clean.png').read_bytes())
        return project, game, engine, modern, {p: p.read_bytes() for p in paths}

    def test_existing_output_is_never_replaced(self):
        project, game, engine, modern, inputs = self.fixture_inputs()
        output = self.root/'existing'
        output.mkdir()
        (output/'keep.txt').write_text('preserve me')
        with mock.patch.object(preparation, 'ROOT', project), \
             mock.patch('fm5_game_files.validate_game_source', return_value={'game_root': str(game)}), \
             mock.patch.object(preparation, 'toolchain', return_value=(project,project,'fixture')), \
             mock.patch.object(preparation, 'run') as runner:
            with self.assertRaisesRegex(ValueError, 'existing files'):
                preparation.prepare(engine, game, modern, output)
        runner.assert_not_called()
        self.assertEqual((output/'keep.txt').read_text(), 'preserve me')
        self.assertEqual({p: p.read_bytes() for p in inputs}, inputs)

    def test_overlap_rejected_before_parent_directories_are_created(self):
        project, game, engine, modern, inputs = self.fixture_inputs()
        output = game/'new/nested/runtime'
        with mock.patch.object(preparation, 'ROOT', project), \
             mock.patch('fm5_game_files.validate_game_source', return_value={'game_root': str(game)}), \
             mock.patch.object(preparation, 'toolchain', return_value=(project,project,'fixture')):
            with self.assertRaisesRegex(ValueError, 'separate'):
                preparation.prepare(engine, game, modern, output)
        self.assertFalse((game/'new').exists())
        self.assertEqual({p: p.read_bytes() for p in inputs}, inputs)

    def test_compiler_failure_cleans_owned_staging_and_does_not_install(self):
        project, game, engine, modern, inputs = self.fixture_inputs()
        output = self.root/'runtime'
        def source_fixture(destination):
            (destination/'tools').mkdir()
            (destination/'src/allocation-port').mkdir(parents=True)
            (destination/'tools/generate_fm5_includes.py').write_text(
                'def generate(*args, **kwargs): return {"fixture": True}\n')
        with mock.patch.object(preparation, 'ROOT', project), \
             mock.patch('fm5_game_files.validate_game_source', return_value={'game_root': str(game)}), \
             mock.patch.object(preparation, 'toolchain', return_value=(project,project,'fixture')), \
             mock.patch.object(preparation, 'copy_authored_source', side_effect=source_fixture), \
             mock.patch.object(preparation, 'run', side_effect=RuntimeError('compiler failed')):
            with self.assertRaisesRegex(RuntimeError, 'compiler failed'):
                preparation.prepare(engine, game, modern, output)
        self.assertFalse(output.exists())
        self.assertFalse(list(self.root.glob('.raceone-prepare-*')))
        self.assertEqual({p: p.read_bytes() for p in inputs}, inputs)


if __name__ == '__main__':
    unittest.main()
