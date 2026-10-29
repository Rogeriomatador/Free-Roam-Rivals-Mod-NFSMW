"""Offline tests for the local read-only Ghidra subsystem call graph."""
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

from tools import ghidra_subsystem_map as graph
from tools import ghidra_research_bridge as bridge


def manifest(count):
    return {
        "schema": "frr-ghidra-bulk-v1",
        "count": count,
        "disk_executable": {
            "md5": bridge.EXPECTED_EXE_MD5,
            "size": bridge.EXPECTED_EXE_BYTES,
        },
        "ghidra_metadata": {"text": "Program Name: speed.exe"},
        "ghidra_program_bytes_verified": False,
        "unresolved_function_entries": [],
    }


def report(address, callers="", callees="", decomp=""):
    return {
        "schema": "frr-ghidra-bulk-v1",
        "address": address,
        "graph_depth": 0,
        "observations": {
            "function_callers": {"text": callers},
            "function_callees": {"text": callees},
            "decompilation": {"text": decomp},
            "xrefs_to": {"text": "DATA @ 0043D388"},
        },
    }


def put_bulk(path, records, source_manifest=None, extra=None):
    m = source_manifest if source_manifest is not None else manifest(len(records))
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("manifest.json", json.dumps(m))
        for address, data in records.items():
            archive.writestr("functions/" + address + ".json", json.dumps(data))
        for name, content in (extra or {}).items():
            archive.writestr(name, content)


class SubsystemMapTests(unittest.TestCase):
    def test_builds_only_observed_directional_calls_and_does_not_export_decomp(self):
        records = {
            "0x0043F040": report("0x0043F040",
                                 callees="FUN_0043ef70 @ 0043ef70"),
            "0x0043EF70": report(
                "0x0043EF70",
                callers="FUN_0043f040 @ 0043f040",
                callees="FUN_0043d330 @ 0043d330",
                decomp="PROPRIETARY CODE not to be copied\nFUN_0043d400 @ 0043d400"),
            "0x0043D330": report("0x0043D330",
                                 callers="FUN_0043ef70 @ 0043ef70"),
        }
        with tempfile.TemporaryDirectory() as folder:
            bulk_zip = Path(folder) / "bulk.zip"
            out = Path(folder) / "graph.zip"
            put_bulk(bulk_zip, records)
            summary, nodes, edges = graph.create_report(bulk_zip)
            self.assertEqual(summary["bulk_queried_addresses"], 3)
            self.assertFalse(summary["ghidra_imported_program_bytes_verified"])
            self.assertFalse(summary["mutation_authorized"])
            self.assertEqual({(e["caller"], e["callee"]) for e in edges}, {
                ("0x0043F040", "0x0043EF70"),
                ("0x0043EF70", "0x0043D330"),
            })
            self.assertTrue(all(e["same_ghidra_project_cross_listing"] for e in edges))
            self.assertTrue(all(not e["independent_runtime_proof"] for e in edges))
            self.assertNotIn("0x0043D400", {n["address"] for n in nodes})
            self.assertNotIn("0x0043D388", {n["address"] for n in nodes})
            graph.save_report(out, summary, nodes, edges)
            with zipfile.ZipFile(out) as archive:
                self.assertEqual(set(archive.namelist()), {
                    "manifest.json", "nodes.json", "edges.json", "LEIA-ME.txt"})
                payload = b"".join(archive.read(k) for k in archive.namelist())
                self.assertNotIn(b"PROPRIETARY", payload)
                self.assertNotIn(b"FUN_0043d400", payload)
            with self.assertRaises(FileExistsError):
                graph.save_report(out, summary, nodes, edges)

    def test_error_endpoints_are_skipped_without_fake_edges(self):
        records = {"0x0043EF70": report("0x0043EF70")}
        records["0x0043EF70"]["observations"]["function_callers"] = {
            "error": "Function not found: 0x0043EF70",
            "text": "FUN_0043f040 @ 0043f040"}
        records["0x0043EF70"]["observations"]["function_callees"] = {
            "text": "Function not found: null"}
        with tempfile.TemporaryDirectory() as folder:
            z = Path(folder) / "bulk.zip"
            put_bulk(z, records)
            summary, nodes, edges = graph.create_report(z)
            self.assertEqual(edges, [])
            self.assertEqual(summary["skipped_missing_or_error_endpoints"], 2)
            self.assertEqual(len(nodes), 1)

    def test_missing_function_listing_is_not_a_negative_proof(self):
        records = {"0x006851D0": {
            "schema": "frr-ghidra-bulk-v1",
            "address": "0x006851D0",
            "observations": {},
        }}
        m = manifest(1)
        m["unresolved_function_entries"] = ["0x006851D0"]
        with tempfile.TemporaryDirectory() as folder:
            z = Path(folder) / "bulk.zip"
            put_bulk(z, records, m)
            summary, nodes, edges = graph.create_report(z)
            self.assertEqual(nodes[0]["query_status"], "unresolved_entry")
            self.assertFalse(nodes[0]["callable_abi_verified"])
            self.assertEqual(edges, [])
            self.assertEqual(summary["bulk_unresolved_entries"], 1)

    def test_atlas_names_add_metadata_but_not_fabricated_edges(self):
        records = {"0x0043F040": report("0x0043F040")}
        with tempfile.TemporaryDirectory() as folder:
            b = Path(folder) / "bulk.zip"
            a = Path(folder) / "atlas.zip"
            put_bulk(b, records)
            atlas = {
                "schema": "frr-nfsmw-ghidra-function-atlas-v1",
                "executable_on_disk": {
                    "size": bridge.EXPECTED_EXE_BYTES,
                    "md5": bridge.EXPECTED_EXE_MD5},
                "ghidra_metadata": {"text": "Program Name: speed.exe"},
                "ghidra_program_bytes_verified": False,
                "function_count": 2,
            }
            internal = {
                "address": "0x0043F040",
                "address_space": "ram",
                "address_full": "ram:0043f040",
                "name": "FUN_0043f040",
                "is_external": False}
            external = {
                "address": "EXTERNAL:00000001",
                "address_space": "EXTERNAL",
                "address_full": "EXTERNAL:00000001",
                "name": "external_api",
                "is_external": True}
            with zipfile.ZipFile(a, "w") as z:
                z.writestr("manifest.json", json.dumps(atlas))
                z.writestr("functions.jsonl",
                           json.dumps(internal) + "\n" + json.dumps(external) + "\n")
            summary, nodes, edges = graph.create_report(b, a)
            self.assertEqual(summary["atlas_function_entries"], 2)
            self.assertEqual(nodes[0]["name_from_optional_atlas"], "FUN_0043f040")
            self.assertEqual(edges, [])

    def test_rejects_wrong_binary_and_false_program_identity(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "bulk.zip"
            bad = manifest(1)
            bad["disk_executable"]["md5"] = "0" * 32
            records = {"0x0043F040": report("0x0043F040")}
            put_bulk(path, records, bad)
            with self.assertRaisesRegex(graph.AtlasError, "supported speed.exe"):
                graph.create_report(path)
            path.unlink()
            bad = manifest(1)
            bad["ghidra_metadata"] = {"text": "Program Name: other.exe"}
            put_bulk(path, records, bad)
            with self.assertRaisesRegex(graph.AtlasError, "metadata"):
                graph.create_report(path)

    def test_rejects_zip_slip_foreign_files_and_duplicate_entries(self):
        records = {"0x0043F040": report("0x0043F040")}
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "bulk.zip"
            put_bulk(path, records, extra={"../speed.exe": "bad"})
            with self.assertRaisesRegex(graph.AtlasError, "unexpected ZIP member"):
                graph.create_report(path)
            path.unlink()
            put_bulk(path, records, extra={"pseudocode.c": "sensitive"})
            with self.assertRaisesRegex(graph.AtlasError, "unexpected ZIP member"):
                graph.create_report(path)

    def test_disputed_interior_is_not_a_function_or_graph_node(self):
        records = {
            "0x0043D330": report(
                "0x0043D330", callees="FUN_0043d388 @ 0043d388"),
        }
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "bulk.zip"
            put_bulk(path, records)
            _, nodes, edges = graph.create_report(path)
            self.assertEqual(edges, [])
            self.assertEqual([x["address"] for x in nodes], ["0x0043D330"])


if __name__ == "__main__":
    unittest.main()
