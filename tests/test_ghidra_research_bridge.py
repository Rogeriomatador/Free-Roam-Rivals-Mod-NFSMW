"""Offline tests: never connect to a real game or Ghidra server."""
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from tools import ghidra_research_bridge as bridge


class FakeResponse:
    def __init__(self, value):
        self.value = value
    def __enter__(self):
        return self
    def __exit__(self, *args):
        return False
    def read(self, limit):
        return self.value[:limit]


class FakeOpener:
    def __init__(self, value=b'{"ok":true}'):
        self.requests = []
        self.value = value
    def open(self, request, timeout):
        self.requests.append((request.full_url, request.get_method(), timeout))
        return FakeResponse(self.value)


class BridgeTests(unittest.TestCase):
    def test_address_guards(self):
        self.assertEqual(bridge.validate_address("0x43EF70"), "0x0043EF70")
        for candidate in ("0x43EF70;rm", "localhost", "0xDEADBEEF", "0x001000"):
            with self.assertRaises(bridge.ResearchError):
                bridge.validate_address(candidate)

    def test_no_post_and_no_external_address(self):
        opener = FakeOpener()
        self.assertEqual(bridge.local_get("/get_xrefs_to", {"address": "0x0043EF70"},
                                          opener=opener), {"ok": True})
        url, method, timeout = opener.requests[0]
        self.assertTrue(url.startswith("http://127.0.0.1:8089/get_xrefs_to?"))
        self.assertEqual(method, "GET")
        self.assertLessEqual(timeout, 15)
        with self.assertRaises(bridge.ResearchError):
            bridge.local_get("/rename_function", opener=opener)
        self.assertEqual(len(opener.requests), 1)

    def test_verify_rejects_wrong_binary_without_network(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "speed.exe"
            path.write_bytes(b"MZ" + b"\0" * 100)
            with self.assertRaisesRegex(bridge.ResearchError, "wrong speed.exe size"):
                bridge.check_executable(path)

    def test_csv_leads_are_sorted_and_unverified(self):
        with tempfile.TemporaryDirectory() as d:
            csv_path = Path(d) / "renames.csv"
            csv_path.write_text(
                "address,name,xref_count,isThunk\n"
                "0043ef70,CreateAIVehicleRacerInstance,23,False\n"
                "0043d388,CreateAIGoalRacerInstance,44,False\n"
                "00400000,CompletelyDifferent,1,False\n", encoding="utf-8")
            rows = bridge.parse_reference_csv(csv_path, "createai", 10)
            self.assertEqual([row["xref_count"] for row in rows], [44, 23])
            self.assertTrue(all(row["status"] == "external_lead_only" for row in rows))

    def test_snapshot_refuses_wrong_ghidra_program_before_code_queries(self):
        seen = []
        def requester(route, params=None):
            seen.append(route)
            return {"programName": "other.exe"}
        with mock.patch.object(bridge, "check_executable", return_value={"supported": True}):
            with self.assertRaisesRegex(bridge.ResearchError, "does not report speed.exe"):
                bridge.take_snapshot(Path("speed.exe"), "0x0043EF70", requester=requester)
        self.assertEqual(seen, ["/mcp/health", "/get_metadata"])

    def test_snapshot_collects_only_allowlisted_read_endpoints(self):
        seen = []
        def requester(route, params=None):
            seen.append((route, params))
            if route == "/get_metadata":
                return {"programName": "speed.exe"}
            return {"items": []}
        with mock.patch.object(bridge, "check_executable",
                               return_value={"supported": True, "md5": bridge.EXPECTED_EXE_MD5}):
            result = bridge.take_snapshot(Path("speed.exe"), "0x0043EF70",
                                          decompile=True, requester=requester)
        self.assertFalse(result["ghidra_program_bytes_verified"])
        self.assertEqual([x[0] for x in seen], [
            "/mcp/health", "/get_metadata", "/get_xrefs_to", "/get_xrefs_from",
            "/decompile_function"])
        self.assertEqual(seen[-1][1]["program"], "speed.exe")

    def test_report_never_overwrites_and_stays_under_ignored_directory(self):
        with tempfile.TemporaryDirectory() as d:
            with mock.patch.object(bridge, "LOCAL_REPORTS", Path(d)):
                p = bridge.new_local_report("report.json", {"result": "ok"})
                self.assertEqual(json.loads(p.read_text())["result"], "ok")
                with self.assertRaises(FileExistsError):
                    bridge.new_local_report("report.json", {"result": "other"})
                with self.assertRaises(bridge.ResearchError):
                    bridge.new_local_report("../escape.json", {})


if __name__ == "__main__":
    unittest.main()
