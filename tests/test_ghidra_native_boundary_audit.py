"""Offline tests for bounded GET-only investigation of native address boundaries."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

from tools import ghidra_native_boundary_audit as audit
from tools import ghidra_research_bridge as bridge


class NativeBoundaryTests(unittest.TestCase):
    def test_all_seeds_valid_and_small_bounded(self):
        self.assertEqual(len(audit.TARGETS), 12)
        addresses = [addr for addr, _ in audit.TARGETS]
        self.assertEqual(len(addresses), len(set(addresses)))
        self.assertIn("0x0043D388", addresses)
        for address in addresses:
            self.assertEqual(bridge.validate_address(address), address)
        for _, route in audit.READ_ROUTES:
            self.assertIn(route, bridge.READ_ONLY_ROUTES)

    def test_get_only_and_one_32_byte_read_per_target(self):
        seen = []
        def fake_request(route, params=None):
            seen.append((route, params))
            return ({"text": "Program Name: speed.exe"}
                    if route == "/get_metadata" else {"text": "test successful"})
        with patch.object(audit.bridge, "check_executable",
                          return_value={"supported": True}):
            manifest, results = audit.capture(
                Path("speed.exe"), requester=fake_request, progress=lambda _: None)
        self.assertEqual(manifest["targets"], len(audit.TARGETS))
        self.assertFalse(manifest["ghidra_program_bytes_verified"])
        self.assertEqual(len(manifest["errors"]), 0)
        self.assertEqual(len(results), len(audit.TARGETS))
        self.assertEqual(len(seen), 2 + 4 * len(audit.TARGETS))
        reads = [(name, params) for name, params in seen if name == "/read_memory"]
        self.assertEqual(len(reads), len(audit.TARGETS))
        self.assertTrue(all(p["length"] == 32 and p["program"] == "speed.exe"
                            for _, p in reads))
        self.assertTrue(all(route in bridge.READ_ONLY_ROUTES
                            for route, _ in seen))

    def test_wrong_program_fails_before_code_query(self):
        seen = []
        def fake_request(route, params=None):
            seen.append(route)
            return {"text": "Program Name: not-the-game.exe"}
        with patch.object(audit.bridge, "check_executable",
                          return_value={"supported": True}):
            with self.assertRaises(bridge.ResearchError):
                audit.capture(Path("speed.exe"), requester=fake_request,
                              progress=lambda _: None)
        self.assertEqual(seen, ["/get_metadata"])

    def test_semantic_and_transport_errors_recorded_without_success_claim(self):
        def fake_request(route, params=None):
            if route == "/get_metadata":
                return {"text": "Program Name: speed.exe"}
            if route == "/get_function_by_address":
                return {"text": "Function not found: null"}
            if route == "/read_memory":
                raise bridge.ResearchError("HTTP 404")
            return {"text": "ok"}
        with patch.object(audit.bridge, "check_executable",
                          return_value={"supported": True}):
            manifest, reports = audit.capture(Path("speed.exe"),
                                               requester=fake_request,
                                               progress=lambda _: None)
        self.assertEqual(len(manifest["errors"]), 2 * len(audit.TARGETS))
        self.assertEqual(reports["0x00689820"]["observations"]["memory"]["error"],
                         "HTTP 404")

    def test_archive_is_new_only_and_contains_no_game_executable(self):
        manifest = {"schema": "test"}
        reports = {"0x0043D388": {"address": "0x0043D388"}}
        with tempfile.TemporaryDirectory() as tmp:
            file = Path(tmp) / "local.zip"
            audit.save_archive(manifest, reports, file)
            with zipfile.ZipFile(file) as zf:
                self.assertEqual(sorted(zf.namelist()), [
                    "LEIA-ME.txt", "addresses/0x0043D388.json", "manifest.json"])
                self.assertEqual(json.loads(zf.read("manifest.json"))["schema"], "test")
            with self.assertRaises(FileExistsError):
                audit.save_archive(manifest, reports, file)


if __name__ == "__main__":
    unittest.main()
