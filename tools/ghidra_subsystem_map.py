#!/usr/bin/env python3
"""Build a read-only, bounded subsystem call graph from LOCAL Ghidra ZIPs.

Consumes the existing ghidra_bulk_research.py and optional ghidra_game_atlas.py
archives. Does not call Ghidra, read a live process, access the internet,
export pseudocode, or modify speed.exe/ASI. Generated data stays under
research/local/ and is intentionally not suitable for authorizing native calls.
"""
import argparse
from collections import defaultdict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys
import zipfile

try:
    from tools import ghidra_research_bridge as bridge
    from tools import ghidra_bulk_research as bulk
except ModuleNotFoundError:
    import ghidra_research_bridge as bridge
    import ghidra_bulk_research as bulk

ADDRESS = re.compile(r"^0x[0-9A-F]{8}$")
BULK_FILE = re.compile(r"^functions/(0x[0-9A-F]{8})[.]json$")
MAX_MEMBERS = 145
MAX_TOTAL = 32 * 1024 * 1024
MAX_MEMBER = 16 * 1024 * 1024

# Semantics are hypotheses curated from FRR's dated research notes, NOT
# symbols recovered from the actual game's source or independently verified.
ANCHORS = {
    "0x0043F040": ("ai", "Racer host allocation candidate"),
    "0x0043EF70": ("ai", "Racer host initialization candidate"),
    "0x0043D400": ("ai", "Racer goal allocation candidate"),
    "0x0043D330": ("ai", "Racer goal initialization candidate"),
    "0x00422480": ("ai", "Goal selection / replacement candidate"),
    "0x0042F140": ("ai", "Racer host update candidate"),
    "0x0042AA80": ("ai", "Goal update candidate"),
    "0x004293D0": ("ai", "Shared AI context candidate"),
    "0x00689820": ("vehicle", "Physical vehicle construction candidate; boundary unresolved"),
    "0x00689020": ("vehicle", "Physical vehicle initialization candidate"),
    "0x006851D0": ("lifecycle", "Retirement candidate; boundary unresolved"),
    "0x006693A0": ("lifecycle", "Activation candidate; boundary unresolved"),
    "0x006693C0": ("lifecycle", "Deactivation candidate; boundary unresolved"),
    "0x006876E0": ("lifecycle", "Driver-class setter candidate; boundary unresolved"),
}
DISPUTED = frozenset((bulk.DISPUTED_ENTRY,))


class AtlasError(ValueError):
    pass


def _archive(path, kind):
    """Validate size, member names and counts before reading untrusted ZIP."""
    if not path.is_file() or path.stat().st_size > MAX_TOTAL:
        raise AtlasError("input ZIP missing or over 32 MiB")
    with zipfile.ZipFile(path) as z:
        infos = z.infolist()
        names = [i.filename for i in infos]
        if len(infos) > MAX_MEMBERS or len(names) != len(set(names)):
            raise AtlasError("duplicate/too many ZIP members")
        allowed = {"manifest.json", "LEIA-ME.txt"}
        if kind == "atlas":
            allowed.add("functions.jsonl")
        if "manifest.json" not in names:
            raise AtlasError("ZIP has no manifest")
        total = 0
        for info in infos:
            name = info.filename
            if (name not in allowed and
                    not (kind == "bulk" and BULK_FILE.fullmatch(name))):
                raise AtlasError("unexpected ZIP member: " + name)
            if info.is_dir() or ((info.external_attr >> 16) & 0o170000) == 0o120000:
                raise AtlasError("directory/symlink ZIP entry rejected")
            if info.file_size > MAX_MEMBER:
                raise AtlasError("oversized ZIP member")
            total += info.file_size
            if total > MAX_TOTAL:
                raise AtlasError("uncompressed ZIP is too large")
        return {name: z.read(name) for name in names if name != "LEIA-ME.txt"}


def _json(raw):
    try:
        return json.loads(raw.decode("utf-8"))
    except (ValueError, UnicodeError) as exc:
        raise AtlasError("malformed JSON/UTF-8 in ZIP") from exc


def _hash(path):
    sha = hashlib.sha256()
    with path.open("rb") as stream:
        for piece in iter(lambda: stream.read(1024 * 1024), b""):
            sha.update(piece)
    return sha.hexdigest()


def _md5(manifest, field):
    obj = manifest.get(field)
    if not isinstance(obj, dict):
        raise AtlasError("missing executable identity: " + field)
    digest = obj.get("md5")
    if not isinstance(digest, str) or digest.upper() != bridge.EXPECTED_EXE_MD5:
        raise AtlasError("ZIP does not identify the supported speed.exe")
    if obj.get("size") != bridge.EXPECTED_EXE_BYTES:
        raise AtlasError("ZIP executable size mismatch")
    return digest.upper()


def read_bulk(path):
    entries = _archive(path, "bulk")
    manifest = _json(entries.pop("manifest.json"))
    if not isinstance(manifest, dict) or manifest.get("schema") != "frr-ghidra-bulk-v1":
        raise AtlasError("wrong Ghidra bulk schema")
    _md5(manifest, "disk_executable")
    if manifest.get("ghidra_program_bytes_verified") is not False:
        raise AtlasError("unexpected Ghidra imported-byte verification claim")
    if not bridge.expected_program_metadata(manifest.get("ghidra_metadata")):
        raise AtlasError("Ghidra metadata does not identify speed.exe")
    reports = {}
    for name, raw in entries.items():
        matched = BULK_FILE.fullmatch(name)
        if not matched:
            raise AtlasError("unexpected archive member")
        report = _json(raw)
        address = matched.group(1)
        if (not isinstance(report, dict) or report.get("schema") != "frr-ghidra-bulk-v1"
                or report.get("address") != address
                or not isinstance(report.get("observations"), dict)):
            raise AtlasError("report address/schema mismatch: " + name)
        reports[address] = report
    if not reports or len(reports) != manifest.get("count"):
        raise AtlasError("bulk manifest count mismatch")
    unresolved = manifest.get("unresolved_function_entries", [])
    if (not isinstance(unresolved, list) or
            any(a not in reports for a in unresolved)):
        raise AtlasError("malformed unresolved entries list")
    return manifest, reports


def read_atlas(path):
    entries = _archive(path, "atlas")
    manifest = _json(entries["manifest.json"])
    if not isinstance(manifest, dict) or manifest.get("schema") != "frr-nfsmw-ghidra-function-atlas-v1":
        raise AtlasError("wrong function atlas schema")
    _md5(manifest, "executable_on_disk")
    if manifest.get("ghidra_program_bytes_verified") is not False:
        raise AtlasError("unexpected Ghidra imported-byte verification claim")
    raw = entries.get("functions.jsonl")
    if raw is None:
        raise AtlasError("function atlas has no functions.jsonl")
    if not bridge.expected_program_metadata(manifest.get("ghidra_metadata")):
        raise AtlasError("function atlas metadata does not identify speed.exe")
    names = {}
    seen = set()
    count = 0
    for line in raw.splitlines():
        row = _json(line)
        if not isinstance(row, dict):
            raise AtlasError("function entry is not an object")
        address, name = row.get("address"), row.get("name")
        if (not isinstance(address, str) or not address
                or not isinstance(name, str) or not 0 < len(name) <= 256
                or type(row.get("is_external")) is not bool):
            raise AtlasError("invalid function atlas entry")
        key = (row.get("address_space"), row.get("address_full"))
        if key in seen:
            raise AtlasError("duplicate address-space entry in function atlas")
        seen.add(key)
        count += 1
        # Ghidra EXTERNAL functions can live in non-image address spaces.
        # Keep the catalog count, but never treat those as game call nodes.
        if row["is_external"]:
            continue
        if not ADDRESS.fullmatch(address) or address in names:
            raise AtlasError("invalid/duplicate internal function address")
        names[address] = name
    if count != manifest.get("function_count"):
        raise AtlasError("function atlas count mismatch")
    return manifest, names


def build_graph(bulk_manifest, reports, atlas_names=None):
    """Only direct function-level caller/callee listings create edges.

    A vtable, xref, decompilation, inferred wrapper or unqueried neighbor
    must NOT be promoted to a direct function call by this tool.
    """
    atlas_names = atlas_names or {}
    edges = defaultdict(set)
    invalid = []
    for address, record in sorted(reports.items()):
        if address in DISPUTED:
            continue
        observations = record["observations"]
        for key in ("function_callers", "function_callees"):
            observation = observations.get(key)
            if observation is None:
                invalid.append((address, key, "missing"))
                continue
            if bulk.response_error(observation):
                invalid.append((address, key, "semantic_error"))
                continue
            if not isinstance(observation, dict) or not isinstance(observation.get("text"), str):
                invalid.append((address, key, "malformed"))
                continue
            for neighbor in bulk.extract_neighbors(observation):
                if neighbor in DISPUTED or neighbor == address:
                    continue
                if key == "function_callers":
                    edges[(neighbor, address)].add("callee_callers_listing")
                else:
                    edges[(address, neighbor)].add("caller_callees_listing")
    unresolved = set(bulk_manifest.get("unresolved_function_entries", []))
    all_nodes = set(reports)
    for a, b in edges:
        all_nodes.update((a, b))
    all_nodes.difference_update(DISPUTED)
    nodes = []
    for address in sorted(all_nodes):
        category, hypothesis = ANCHORS.get(address, ("unclassified", None))
        nodes.append({
            "address": address,
            "subsystem_hypothesis": category,
            "semantic_hypothesis": hypothesis,
            "name_from_optional_atlas": atlas_names.get(address),
            "query_status": ("unresolved_entry" if address in unresolved else
                             "queried" if address in reports else "neighbor_only"),
            "callable_abi_verified": False,
        })
    relations = [{
        "caller": caller,
        "callee": callee,
        "observed_via": sorted(sources),
        "same_ghidra_project_cross_listing": len(sources) == 2,
        "independent_runtime_proof": False,
    } for (caller, callee), sources in sorted(edges.items())]
    return nodes, relations, invalid


def create_report(bulk_path, atlas_path=None):
    manifest, reports = read_bulk(bulk_path)
    atlas_manifest, names = (read_atlas(atlas_path) if atlas_path else (None, {}))
    nodes, edges, skipped = build_graph(manifest, reports, names)
    summary = {
        "schema": "frr-ghidra-subsystem-map-v1",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "supported_disk_exe_md5": bridge.EXPECTED_EXE_MD5,
        "bulk_sha256": _hash(bulk_path),
        "atlas_sha256": _hash(atlas_path) if atlas_path else None,
        "bulk_queried_addresses": len(reports),
        "bulk_unresolved_entries": len(manifest.get("unresolved_function_entries", [])),
        "atlas_function_entries": atlas_manifest["function_count"] if atlas_manifest else None,
        "graph_nodes": len(nodes),
        "direct_listed_call_edges": len(edges),
        "skipped_missing_or_error_endpoints": len(skipped),
        "skipped_details": [{"address": a, "endpoint": k, "reason": r} for a, k, r in skipped],
        "ghidra_imported_program_bytes_verified": False,
        "source_type": "static Ghidra function listings; not independent verification",
        "contains_game_binary": False,
        "contains_decompiled_game_code": False,
        "mutation_authorized": False,
        "limitations": [
            "Only sampled direct caller/callee results; indirect dispatch and vtables are not complete.",
            "Both listings derive from one Ghidra project, not independent confirmations.",
            "No runtime identity, calling convention, lifetime or API ownership is established.",
            "Original Ghidra imported bytes have not been matched to the local disk EXE.",
            "Read-only hypothesis inventory: do not call, hook, patch or retarget handles based on this report.",
        ],
    }
    return summary, nodes, edges


def save_report(path, summary, nodes, edges):
    path = Path(path)
    if path.suffix.lower() != ".zip":
        raise AtlasError("output must end in .zip")
    path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(path, "x", compression=zipfile.ZIP_DEFLATED) as archive:
        for name, data in (
                ("manifest.json", summary),
                ("nodes.json", nodes),
                ("edges.json", edges)):
            archive.writestr(name, json.dumps(data, indent=2, ensure_ascii=False) + "\n")
        archive.writestr("LEIA-ME.txt",
                         "FRR call graph: local, read-only static research.\n"
                         "No EXE, ASI or Ghidra decompilation is exported.\n"
                         "Do not publish private raw research ZIPs.\n"
                         "This map NEVER authorizes native calls or handle rewrites.\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--bulk", type=Path, required=True,
                        help="local ZIP from ghidra_bulk_research.py")
    parser.add_argument("--atlas", type=Path,
                        help="optional local ZIP from ghidra_game_atlas.py")
    parser.add_argument("--output", type=Path, help="new ZIP; never overwrite")
    args = parser.parse_args(argv)
    try:
        summary, nodes, edges = create_report(args.bulk, args.atlas)
        target = args.output or (bridge.LOCAL_REPORTS /
            ("ghidra_subsystems_" + datetime.now().strftime("%Y%m%d-%H%M%S-%f") + ".zip"))
        save_report(target, summary, nodes, edges)
    except (AtlasError, zipfile.BadZipFile, OSError, ValueError) as exc:
        parser.error(str(exc))
    print("Local read-only subsystem atlas: " + str(target))
    print("Queried: %d | nodes: %d | direct listed edges: %d | skipped endpoints: %d" %
          (summary["bulk_queried_addresses"], len(nodes), len(edges),
           summary["skipped_missing_or_error_endpoints"]))
    print("No live game or binary modified; imported Ghidra program bytes still unverified.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
