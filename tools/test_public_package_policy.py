"""Headless policy fixtures only. Never starts the builder, host or a DLL."""
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import public_package_policy as policy


class PublicPackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="race-one-policy-")
        self.root = Path(self.temp.name) / "Public Builder é space"
        self.root.mkdir()
        (self.root / "RaceOneHub.exe").write_bytes(b"fixture-owned-builder-not-an-executable")
        (self.root / "LICENSE").write_text("fixture license", encoding="utf-8")
        self.manifest = {"schema": 1, "scope": "initial-public-builder", "release_blockers": ["External runtime route is not integrated."], "files": []}
        for p in self.root.iterdir():
            data = p.read_bytes()
            self.manifest["files"].append({"path": p.name, "bytes": len(data),
                "sha256": hashlib.sha256(data).hexdigest(), "origin": "Headless synthetic fixture"})

    def tearDown(self):
        self.temp.cleanup()

    def test_containment_does_not_clear_release_blockers(self):
        before = {p.name: p.read_bytes() for p in self.root.iterdir()}
        report = policy.audit_public_tree(self.root, self.manifest)
        self.assertTrue(report["containment_passed"])
        self.assertFalse(report["release_ready"])
        self.assertEqual(report["files_verified"], 2)
        self.assertEqual(before, {p.name: p.read_bytes() for p in self.root.iterdir()})

    def test_explicit_cleared_fixture_blockers(self):
        self.manifest["release_blockers"] = []
        self.assertTrue(policy.audit_public_tree(self.root, self.manifest)["release_ready"])

    def test_hash_change_same_length(self):
        p = self.root / "LICENSE"
        p.write_bytes(b"X" + p.read_bytes()[1:])
        with self.assertRaises(policy.PackagePolicyError):
            policy.audit_public_tree(self.root, self.manifest)

    def test_size_change(self):
        with (self.root / "LICENSE").open("ab") as f:
            f.write(b"x")
        with self.assertRaises(policy.PackagePolicyError):
            policy.audit_public_tree(self.root, self.manifest)

    def test_missing_file(self):
        (self.root / "LICENSE").unlink()
        with self.assertRaises(policy.PackagePolicyError):
            policy.audit_public_tree(self.root, self.manifest)

    def test_unknown_file(self):
        (self.root / "extra.dll").write_bytes(b"unreviewed")
        with self.assertRaises(policy.PackagePolicyError):
            policy.audit_public_tree(self.root, self.manifest)

    def test_unknown_empty_folder(self):
        (self.root / "unlisted").mkdir()
        with self.assertRaises(policy.PackagePolicyError):
            policy.audit_public_tree(self.root, self.manifest)

    def test_private_names_denied_even_if_allowlisted(self):
        for name in sorted(policy.FORBIDDEN_NAMES):
            with self.subTest(name=name), self.assertRaises(policy.PackagePolicyError):
                policy._relative("components/" + name)

    def test_private_folders_and_suffixes(self):
        for part in sorted(policy.FORBIDDEN_PARTS):
            with self.subTest(part=part), self.assertRaises(policy.PackagePolicyError):
                policy._relative(part + "/payload.bin")
        for suffix in sorted(policy.FORBIDDEN_SUFFIXES):
            with self.subTest(suffix=suffix), self.assertRaises(policy.PackagePolicyError):
                policy._relative("anything" + suffix)

    def test_traversal_windows_and_normalization(self):
        for name in ["../secret", "a/../b", "/absolute", "C:/secret", "C:relative", "a\\b", "a//b", "./a", "a/./b", "a/", "a./b", "a /b", ""]:
            with self.subTest(name=name), self.assertRaises(policy.PackagePolicyError):
                policy._relative(name)

    def test_game_and_runtime_originals_denied_under_renamed_files(self):
        for digest in sorted(policy.EXTERNAL_HASHES):
            manifest = deepcopy(self.manifest)
            for f in manifest["files"]:
                f["sha256"] = digest
            with patch.object(policy, "_sha", return_value=digest):
                with self.assertRaises(policy.PackagePolicyError):
                    policy.audit_public_tree(self.root, manifest)

    def test_explicit_case_collision(self):
        self.manifest["files"].append(dict(self.manifest["files"][0], path="raceonehub.EXE"))
        with self.assertRaises(policy.PackagePolicyError):
            policy.audit_public_tree(self.root, self.manifest)

    def test_manifest_requires_complete_provenance(self):
        for key, value in [("sha256", "a" * 63), ("sha256", "G" * 64), ("bytes", True), ("bytes", -1), ("origin", "")]:
            manifest = deepcopy(self.manifest)
            manifest["files"][0][key] = value
            with self.subTest(key=key, value=value), self.assertRaises(policy.PackagePolicyError):
                policy.audit_public_tree(self.root, manifest)

    def test_wrong_scope_cannot_audit_generated_game_output(self):
        self.manifest["scope"] = "private-game-output"
        with self.assertRaises(policy.PackagePolicyError):
            policy.audit_public_tree(self.root, self.manifest)

    def test_external_input_only_reads(self):
        p = self.root / "LICENSE"
        expected = {"id": "fixture-external", "bytes": p.stat().st_size, "sha256": hashlib.sha256(p.read_bytes()).hexdigest()}
        before = p.read_bytes()
        report = policy.validate_external_input(p, expected)
        self.assertTrue(report["validated"])
        self.assertFalse(report["loaded"])
        self.assertFalse(report["copied"])
        self.assertEqual(before, p.read_bytes())
        expected["sha256"] = "0" * 64
        with self.assertRaises(policy.PackagePolicyError):
            policy.validate_external_input(p, expected)


if __name__ == "__main__":
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(PublicPackageTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    print(json.dumps({"passed": result.wasSuccessful(), "test_methods": result.testsRun,
                      "scope": "Synthetic ordinary files only; no application/DLL execution or archive assembly."}))
    raise SystemExit(0 if result.wasSuccessful() else 1)
