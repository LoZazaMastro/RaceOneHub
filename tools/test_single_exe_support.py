from pathlib import Path
import json
import tempfile
import unittest
import zipfile
from build_single_exe_support import package


class ContainerPackagingTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.root=Path(self.temp.name)
        self.input=self.root/'fixture.txt';self.input.write_bytes(b'AUTHORED NONEXECUTABLE FIXTURE')
        self.mapping=[{'path':'tools/marker.txt','source':str(self.input),'origin':'authored'}]

    def test_exact_manifest_and_reproducible_archive(self):
        first=package(self.mapping,self.root/'one.zip')
        second=package(self.mapping,self.root/'two.zip')
        self.assertEqual(first['sha256'],second['sha256'])
        with zipfile.ZipFile(first['archive']) as archive:
            self.assertEqual(set(archive.namelist()),{'manifest.json','tools/marker.txt'})
            self.assertEqual(archive.read('tools/marker.txt'),self.input.read_bytes())
            self.assertNotIn(str(self.root),archive.read('manifest.json').decode())
            self.assertEqual(json.loads(archive.read('manifest.json')),first['manifest'])

    def test_invalid_names_never_create_output(self):
        for name in ['../escape','/absolute','C:/outside','tools\\outside','tools/../outside','tools/trailing.','manifest.json','tools/CON.txt']:
            with self.subTest(name=name):
                with self.assertRaises(ValueError):
                    package([dict(self.mapping[0],path=name)],self.root/'bad.zip')
                self.assertFalse((self.root/'bad.zip').exists())

    def test_duplicate_case_insensitive_names_refused(self):
        with self.assertRaises(ValueError):
            package([*self.mapping,dict(self.mapping[0],path='TOOLS/Marker.txt')],self.root/'bad.zip')

    def test_provenance_is_required(self):
        with self.assertRaises(ValueError):
            package([dict(self.mapping[0],origin='unspecified')],self.root/'bad.zip')

    def test_existing_output_is_unchanged(self):
        output=self.root/'keep.zip';output.write_bytes(b'KEEP')
        with self.assertRaises(ValueError):package(self.mapping,output)
        self.assertEqual(output.read_bytes(),b'KEEP')


if __name__=='__main__':unittest.main()
