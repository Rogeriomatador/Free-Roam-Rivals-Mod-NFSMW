"""Offline tests only: never connects to game, Ghidra or external networks."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

from tools import ghidra_game_atlas as atlas
from tools import ghidra_research_bridge as bridge


def function(address, name=None, external=False):
    return {"address": "%08x" % address,
            "name": name or "FUN_%08x" % address,
            "isThunk": False, "isExternal": external}


class AtlasTests(unittest.TestCase):
    def test_normalizes_enhanced_records_without_inventing_signatures(self):
        row = atlas.normalize_function(function(0x43D330))
        self.assertEqual(row["address"], "0x0043D330")
        self.assertEqual(row["address_full"], "0x0043D330")
        self.assertTrue(row["name"].startswith("FUN_"))
        self.assertNotIn("signature", row)
        self.assertNotIn("pseudocode", row)
        with self.assertRaises(bridge.ResearchError):
            atlas.normalize_function({"address": "0043D330", "name": "n"})
        with self.assertRaises(bridge.ResearchError):
            atlas.normalize_function(function(0x43D330) | {"isThunk": "false"})

    def test_collect_paginates_all_recognized_entries_and_checks_metadata_count(self):
        entries = [function(0x401000 + 16 * i) for i in range(52)]
        entries[0]["name"] = "CreateRacer"
        observed = []
        def fake_request(route, params=None):
            observed.append((route, params))
            if route == "/get_metadata":
                return {"text": "Program Name: speed.exe\nFunction Count: 52"}
            if route == "/mcp/health":
                return {"status": "ok"}
            if route == "/list_functions_enhanced":
                offset, limit = params["offset"], params["limit"]
                chunk = entries[offset:offset+limit]
                return {"functions": chunk, "offset": offset,
                        "limit": limit, "count": len(chunk)}
            raise AssertionError("Unexpected endpoint")
        with patch.object(atlas.bridge, "check_executable",
                          return_value={"md5": bridge.EXPECTED_EXE_MD5}):
            manifest, catalog = atlas.collect(
                Path("speed.exe"), page_size=50,
                requester=fake_request, progress=lambda _: None)
        self.assertEqual(manifest["function_count"], 52)
        self.assertEqual(manifest["reported_function_count"], 52)
        self.assertEqual(manifest["pages_used"], 2)
        self.assertFalse(manifest["ghidra_program_bytes_verified"])
        self.assertFalse(manifest["contains_game_decompilation"])
        self.assertEqual(manifest["counts"]["named"], 1)
        self.assertEqual(manifest["counts"]["auto_named"], 51)
        rows = [json.loads(line) for line in catalog.decode("utf-8").splitlines()]
        self.assertEqual(len(rows), 52)
        self.assertTrue(all(set(row) == {
            "address", "address_full", "address_space", "name", "is_thunk", "is_external"
        } for row in rows))
        self.assertEqual([name for name, _ in observed],
                         ["/get_metadata", "/mcp/health",
                          "/list_functions_enhanced", "/list_functions_enhanced"])
        self.assertTrue(all(name in bridge.READ_ONLY_ROUTES for name, _ in observed))
        self.assertEqual(observed[-1][1]["program"], "speed.exe")

    def test_count_mismatch_and_duplicate_addresses_fail_closed(self):
        def requester(route, params=None):
            if route == "/get_metadata":
                return {"text": "Program Name: speed.exe\nFunction Count: 51"}
            if route == "/mcp/health":
                return {"status": "ok"}
            if route == "/list_functions_enhanced":
                addr = function(0x0043EF70)
                return {"functions": [addr] if params["offset"] == 0 else [],
                        "count": 1 if params["offset"] == 0 else 0,
                        "offset": params["offset"], "limit": params["limit"]}
        with patch.object(atlas.bridge, "check_executable",
                          return_value={"supported": True}):
            with self.assertRaisesRegex(bridge.ResearchError, "incomplete"):
                atlas.collect(Path("speed.exe"), page_size=50,
                              requester=requester, progress=lambda _: None)
        def duplicate(route, params=None):
            if route == "/get_metadata":
                return {"text": "Program Name: speed.exe"}
            if route == "/mcp/health":
                return {"status": "ok"}
            if route == "/list_functions_enhanced":
                rows = [function(0x401000)] * 50
                return {"functions": rows, "count": 50,
                        "offset": params["offset"], "limit": params["limit"]}
        with patch.object(atlas.bridge, "check_executable",
                          return_value={"supported": True}):
            with self.assertRaisesRegex(bridge.ResearchError, "duplicate"):
                atlas.collect(Path("speed.exe"), page_size=50,
                              requester=duplicate, progress=lambda _: None)

    def test_wrong_binary_rejected_without_any_network(self):
        with patch.object(atlas.bridge, "check_executable",
                          side_effect=bridge.ResearchError("wrong speed.exe MD5")):
            def should_not_call(*args):
                raise AssertionError("no Ghidra calls expected")
            with self.assertRaisesRegex(bridge.ResearchError, "wrong speed.exe MD5"):
                atlas.collect(Path("speed.exe"), requester=should_not_call,
                              progress=lambda _: None)

    def test_bad_pagination_shape_and_semantic_error_are_rejected(self):
        for response in (
                {"error": "Unknown program"},
                {"functions": [], "count": 20, "offset": 0, "limit": 50},
                {"functions": [function(0x401000)], "count": 1,
                 "offset": 200, "limit": 50},
                {"functions": "unexpected", "count": 1, "offset": 0}):
            with self.assertRaises(bridge.ResearchError):
                atlas.read_page(lambda route, params: response, 0, 50)

    def test_zip_contains_only_metadata_and_is_searchable_offline(self):
        with tempfile.TemporaryDirectory() as dirname:
            path = Path(dirname) / "atlas.zip"
            manifest = {"schema": "frr-nfsmw-ghidra-function-atlas-v1",
                        "program": "speed.exe"}
            catalog = (json.dumps(atlas.normalize_function(
                function(0x43EF70, name="CreateRacer"))) + "\n").encode("utf-8")
            atlas.save_zip(path, manifest, catalog)
            with zipfile.ZipFile(path) as archive:
                self.assertEqual(set(archive.namelist()), {
                    "manifest.json", "functions.jsonl", "LEIA-ME.txt"})
                self.assertFalse(any(name.endswith((".exe", ".asi", ".dll"))
                                     for name in archive.namelist()))
            result = atlas.search_zip(path, phrase="racer")
            self.assertEqual(result["results"][0]["address"], "0x0043EF70")
            result = atlas.search_zip(path, address="0x0043EF70")
            self.assertEqual(len(result["results"]), 1)
            self.assertEqual(atlas.search_zip(path, phrase="nope")["results"], [])
            with self.assertRaises(FileExistsError):
                atlas.save_zip(path, manifest, catalog)


if __name__ == "__main__":
    unittest.main()
