import hashlib
import importlib.util
import os
import struct
import unittest
import tempfile
from pathlib import Path
from unittest.mock import patch

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location(
    "dump_exe_functions", os.path.join(HERE, "..", "tools", "dump_exe_functions.py"))
dump = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dump)


def synthetic_pe(code_at_rva, code, text_rva=0x1000, raw=0x400, size=0x2000):
    """Minimal PE32 with one .text section mapped at image_base + text_rva."""
    image = bytearray(raw + size)
    image[:2] = b"MZ"
    struct.pack_into("<I", image, 0x3C, 0x80)
    pe = 0x80
    image[pe:pe + 4] = b"PE\0\0"
    struct.pack_into("<HHIIIHH", image, pe + 4, 0x14C, 1, 0, 0, 0, 224, 0x102)
    opt = pe + 24
    struct.pack_into("<H", image, opt, 0x10B)
    struct.pack_into("<I", image, opt + 28, 0x400000)
    sec = opt + 224
    image[sec:sec + 8] = b".text\0\0\0"
    struct.pack_into("<IIII", image, sec + 8, size, text_rva, size, raw)
    off = raw + (code_at_rva - text_rva)
    image[off:off + len(code)] = code
    return bytes(image)


class DumpTests(unittest.TestCase):
    def setUp(self):
        self.orig = (dump.SUPPORTED_SIZE, dump.SUPPORTED_MD5, dump.TARGETS)

    def tearDown(self):
        dump.SUPPORTED_SIZE, dump.SUPPORTED_MD5, dump.TARGETS = self.orig

    def make(self, code):
        data = synthetic_pe(0x1100, code)
        dump.SUPPORTED_SIZE = len(data)
        dump.SUPPORTED_MD5 = hashlib.md5(data).hexdigest().upper()
        dump.TARGETS = {"fn": (0x401100, "test")}
        return data

    def test_va_mapping_and_ret_imm(self):
        data = self.make(bytes([0x55, 0x8B, 0xEC, 0xC2, 0x0C, 0x00]))
        pe = dump.PE(data)
        off, sec = pe.va_to_offset(0x401100)
        self.assertEqual(sec, ".text")
        self.assertEqual(data[off], 0x55)
        report = dump.build_report("x.exe", data)
        self.assertIn("ABI verification: NOT performed", report)
        self.assertIn("55 8b ec c2 0c 00", report)

    def test_bytes_option(self):
        data = self.make(bytes(range(64)))
        self.assertEqual(dump.build_report("x.exe", data, 16).count("   +0"), 1)
        self.assertGreater(dump.build_report("x.exe", data, 64).count("   +0"), 3)

    def test_return_byte_inside_instruction_does_not_claim_abi(self):
        # cmp eax, ebx is 3b c3, not a RET; the old byte scan mislabelled it.
        report = dump.build_report("x.exe", self.make(bytes([0x3B, 0xC3, 0x90])))
        self.assertNotIn("calleePops=", report)
        self.assertNotIn("firstReturnHeuristic=", report)
        self.assertIn("bytesPerTarget=192", report)

    def test_unmapped_va_reported(self):
        data = self.make(b"\xC3")
        dump.TARGETS = {"fn": (0x900000, "test")}
        self.assertIn("NOT MAPPED", dump.build_report("x.exe", data))

    def test_refuses_unsupported_exe(self):
        data = self.make(b"\xC3")
        dump.SUPPORTED_MD5 = "0" * 32
        with self.assertRaises(SystemExit):
            dump.build_report("x.exe", data)

    def test_rejects_non_pe(self):
        with self.assertRaises(ValueError):
            dump.PE(b"not a pe at all" * 20)

    def test_refuses_overwriting_executable_or_existing_report(self):
        with tempfile.TemporaryDirectory() as root:
            exe = Path(root) / "speed.exe"
            exe.write_bytes(b"original game")
            report = Path(root) / "report.txt"
            report.write_bytes(b"existing report")
            for output in (exe, report):
                with self.assertRaises(SystemExit), patch("sys.stderr"):
                    dump.main([str(exe), "-o", str(output)])
            self.assertEqual(exe.read_bytes(), b"original game")
            self.assertEqual(report.read_bytes(), b"existing report")

    def test_hardlink_output_does_not_modify_input(self):
        with tempfile.TemporaryDirectory() as root:
            exe = Path(root) / "speed.exe"
            exe.write_bytes(b"original game")
            link = Path(root) / "alias.txt"
            os.link(exe, link)
            with self.assertRaises(SystemExit), patch("sys.stderr"):
                dump.main([str(exe), "-o", str(link)])
            self.assertEqual(exe.read_bytes(), b"original game")

    def test_short_section_is_reported_and_not_crossed(self):
        data = self.make(b"\xc3")
        dump.TARGETS = {"end": (0x402FF0, "test")}
        report = dump.build_report("x.exe", data, 64)
        self.assertIn("actualBytes=16 truncated=yes", report)

    def test_writes_new_report_without_changing_input(self):
        data = self.make(b"\xc3")
        with tempfile.TemporaryDirectory() as root:
            exe = Path(root) / "speed.exe"
            output = Path(root) / "report.txt"
            exe.write_bytes(data)
            with patch("sys.stdout"):
                self.assertEqual(dump.main([str(exe), "-o", str(output)]), 0)
            self.assertEqual(exe.read_bytes(), data)
            self.assertIn("FRR_EXE_DUMP_V2", output.read_text(encoding="utf-8"))


if __name__ == "__main__":
    unittest.main()
