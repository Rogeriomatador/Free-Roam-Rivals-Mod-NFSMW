import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

spec=importlib.util.spec_from_file_location('collector',Path(__file__).resolve().parents[1]/'tools/collect_diagnostic_bundle.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)

class BundleTests(unittest.TestCase):
    def game(self, root):
        (root/'speed.exe').write_bytes(b'fake executable for synthetic test')
        folder=root/'scripts'/'FreeRoamRivals';folder.mkdir(parents=True)
        return folder
    def test_collect_no_binaries_or_saves(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);folder=self.game(root)
            (root/'plugin.dll').write_bytes(b'plugin')
            (root/'save.bin').write_bytes(b'private')
            (folder/'FreeRoamRivals.ini').write_text('[Diagnostics]\nDiagnosticBundleEnabled=1')
            (folder/'FreeRoamRivals.log').write_text('NativePrototype native construction request\n')
            (folder/'NativeExceptions.log').write_bytes(b'FIRST_CHANCE code=0xc0000005\n')
            report=m.collect(root,root/'result.zip')
            self.assertFalse(report['supported_executable'])
            self.assertEqual(report['summary']['constructor_invocations'],0)
            with zipfile.ZipFile(root/'result.zip') as z:
                self.assertEqual(set(z.namelist()),{'diagnostic_report.json','config/FreeRoamRivals.ini','FreeRoamRivals.log.tail','NativeExceptions.log'})
                self.assertEqual(z.read('NativeExceptions.log'),b'FIRST_CHANCE code=0xc0000005\n')
            self.assertIn('not proof of a fatal crash',report['exception_trace']['scope'])
            self.assertEqual(len(report['files']),2)
    def test_existing_zip_never_overwritten(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);self.game(root);out=root/'result.zip';out.write_bytes(b'keep')
            with self.assertRaises(FileExistsError):m.collect(root,out)
            self.assertEqual(out.read_bytes(),b'keep')
    def test_bounded_log_tail(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);folder=self.game(root)
            (folder/'FreeRoamRivals.log').write_bytes(b'old\n'+b'x\n'*30+b'NativeFactory constructor invoke\n')
            old=m.MAX_LOG
            try:
                m.MAX_LOG=48;report=m.collect(root,root/'result.zip')
            finally:m.MAX_LOG=old
            self.assertTrue(report['log']['truncated']);self.assertLessEqual(report['log']['collected_bytes'],48)
            self.assertEqual(report['summary']['constructor_invocations'],1)
    def test_hash_budget_and_missing_log(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);self.game(root);old=m.MAX_HASH_BYTES
            try:
                m.MAX_HASH_BYTES=0;report=m.collect(root,root/'result.zip')
            finally:m.MAX_HASH_BYTES=old
            self.assertIn('hash_skipped',report['files'][0]);self.assertTrue(report['warnings'])
    def test_mismatch_and_native_activation_are_not_render_proof(self):
        summary=m.summarize('NativeFactory compatibility audit address=0x422480 match=0\nNativePrototype stage=active\n')
        self.assertEqual(len(summary['compatibility_mismatches']),1)
        self.assertEqual(summary['activation_records'],1)
        self.assertTrue(any('presença visual' in x for x in summary['next_checks']))
    def test_hash_read_is_bounded(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp)/"growing.dll";p.write_bytes(b"abcdef")
            result=m.fingerprint(p,3)
            self.assertEqual(result["bytes_hashed"],3)
            self.assertTrue(result["changed_during_read"])
    def test_bad_directory(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(ValueError):m.collect(tmp,Path(tmp)/'result.zip')
    def test_exception_trace_size_limit(self):
        with tempfile.TemporaryDirectory() as tmp:
            root=Path(tmp);folder=self.game(root)
            (folder/'NativeExceptions.log').write_bytes(b'x'*(m.MAX_EXCEPTION_TRACE+1))
            report=m.collect(root,root/'result.zip')
            self.assertNotIn('exception_trace',report)
            self.assertTrue(any('128 KiB' in warning for warning in report['warnings']))
            with zipfile.ZipFile(root/'result.zip') as z:
                self.assertNotIn('NativeExceptions.log',z.namelist())

if __name__=='__main__':unittest.main()
