"""Offline regression tests for the single-ZIP bounded Ghidra bulk collector."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import zipfile

from tools import ghidra_bulk_research as bulk
from tools import ghidra_research_bridge as bridge


class BulkTests(unittest.TestCase):
    def test_seeds_are_valid_and_disputed_interior_not_included(self):
        self.assertGreater(len(bulk.SEEDS), 15)
        self.assertEqual(len(set(bulk.SEEDS)), len(bulk.SEEDS))
        self.assertNotIn(bulk.DISPUTED_ENTRY, bulk.SEEDS)
        for address in bulk.SEEDS:
            self.assertEqual(bridge.validate_address(address), address)

    def test_extraction_uses_function_listings_not_arbitrary_strings(self):
        blob = {"text": ("FUN_0043d330 @ 0043d330\n"
                         "FUN_0043d330 @ 0043d330\n"
                         "FUN_0043d388 @ 0043d388\n"
                         "FUN_00999999 @ 00999999\n"
                         "From 0043d400 in function X\n")}
        self.assertEqual(bulk.extract_neighbors(blob),
                         ("0x0043D330", "0x00999999"))
        self.assertEqual(bulk.extract_neighbors({"text": "No callers found for function: null"}), ())
        self.assertEqual(bulk.extract_neighbors({"error": "timeout"}), ())

    def test_collect_read_only_auto_discovers_and_honors_caps(self):
        observed = []
        def fake_request(route, params=None):
            observed.append((route, params))
            if route == "/get_metadata":
                return {"text": "Program Name: speed.exe"}
            if route == "/get_function_callees":
                return {"text": "FUN_00407e10 @ 00407e10\nFUN_0043d388 @ 0043d388"}
            if route == "/get_function_callers":
                return {"text": "FUN_005d29d0 @ 005d29d0"}
            return {"text": "ok"}
        with mock.patch.object(bulk.bridge, "check_executable",
                               return_value={"md5": bridge.EXPECTED_EXE_MD5}):
            manifest, reports = bulk.collect(
                Path("speed.exe"), max_functions=len(bulk.SEEDS) + 2,
                depth=1, max_decompile=2, requester=fake_request,
                progress=lambda _: None)
        self.assertEqual(len(reports), len(bulk.SEEDS) + 2)
        self.assertFalse(manifest["ghidra_program_bytes_verified"])
        self.assertEqual(manifest["decompiled"], 2)
        self.assertEqual(len(manifest["errors"]), 0)
        self.assertIn("0x00407E10", reports)
        self.assertIn("0x005D29D0", reports)
        self.assertNotIn(bulk.DISPUTED_ENTRY, reports)
        self.assertEqual(sum(route == "/decompile_function"
                             for route, _ in observed), 2)
        self.assertEqual(sum(route == "/get_metadata" for route, _ in observed), 1)
        self.assertFalse(any(route not in bridge.READ_ONLY_ROUTES
                             for route, _ in observed))
        self.assertEqual(reports["0x00407E10"]["graph_depth"], 1)

    def test_guard_rejects_excessive_limits_without_any_queries(self):
        with mock.patch.object(bulk.bridge, "check_executable") as checker:
            for count, depth, decomp in (
                (10000, 1, 1), (60, 5, 1), (60, 1, 100),
                (len(bulk.SEEDS) - 1, 1, 0),
            ):
                with self.assertRaises(bridge.ResearchError):
                    bulk.collect(Path("speed.exe"), count, depth, decomp,
                                 progress=lambda _: None)
            checker.assert_not_called()

    def test_rejects_wrong_program_without_function_queries(self):
        requests = []
        def fake_request(route, params=None):
            requests.append(route)
            return {"text": "Program Name: wrong.exe"}
        with mock.patch.object(bulk.bridge, "check_executable",
                               return_value={"supported": True}):
            with self.assertRaisesRegex(bridge.ResearchError,
                                         "did not report speed.exe"):
                bulk.collect(Path("speed.exe"), progress=lambda _: None,
                             requester=fake_request)
        self.assertEqual(requests, ["/get_metadata"])

    def test_partial_get_errors_do_not_lose_all_results(self):
        def fake_request(route, params=None):
            if route == "/get_metadata":
                return {"text": "Program Name: speed.exe"}
            if route == "/get_function_callees":
                raise bridge.ResearchError("timeout")
            return {"text": "ok"}
        with mock.patch.object(bulk.bridge, "check_executable",
                               return_value={"supported": True}):
            m, reps = bulk.collect(Path("speed.exe"),
                                   max_functions=len(bulk.SEEDS),
                                   depth=0, max_decompile=0,
                                   requester=fake_request,
                                   progress=lambda _: None)
        self.assertEqual(len(reps), len(bulk.SEEDS))
        self.assertEqual(len(m["errors"]), len(bulk.SEEDS))
        self.assertEqual(m["decompiled"], 0)

    def test_zip_is_single_artifact_with_no_executable_and_never_overwrites(self):
        manifest = {"schema": "test", "count": 1}
        reports = {"0x0043D330": {"address": "0x0043D330",
                                   "observations": {"text": "sample"}}}
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "batch.zip"
            bulk.make_zip(manifest, reports, path)
            with zipfile.ZipFile(path) as zf:
                self.assertEqual(set(zf.namelist()), {
                    "manifest.json", "functions/0x0043D330.json",
                    "LEIA-ME.txt"})
                self.assertEqual(json.loads(zf.read("manifest.json"))["count"], 1)
                self.assertEqual(json.loads(zf.read(
                    "functions/0x0043D330.json"))["address"], "0x0043D330")
                self.assertFalse(any(".exe" in s or ".asi" in s
                                     for s in zf.namelist()))
            with self.assertRaises(FileExistsError):
                bulk.make_zip(manifest, reports, path)


if __name__ == "__main__":
    unittest.main()
