#!/usr/bin/env python3
"""Collect a bounded Ghidra MCP research neighborhood into ONE local ZIP.

No game process/binary mutation, no remote endpoints, no automatic Git uploads.
Ghidra must already be open with speed.exe and the MCP plugin running locally.
"""
import argparse
from collections import deque
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import sys
import zipfile

try:
    from tools import ghidra_research_bridge as bridge
except ModuleNotFoundError:
    # Also support direct invocation: py -3 .\\tools\\ghidra_bulk_research.py
    import ghidra_research_bridge as bridge

# Curated hypotheses from FRR research; none is authorization for native calls.
# Explicitly omit 0x43D388: its function boundary is disputed.
SEEDS = (
    "0x0043F040", "0x0043EF70", "0x0043D400", "0x0043D330",
    "0x00422480", "0x00423010", "0x00423190", "0x00423510",
    "0x0042AB80", "0x0042F140", "0x004293D0", "0x0042AA80",
    "0x0043B0D0", "0x0043CFF0", "0x00689820", "0x00689020",
    "0x006851D0", "0x00688120", "0x006693A0", "0x006693C0",
    "0x00415D00", "0x00422690", "0x006876E0",
)
DISPUTED_ENTRY = "0x0043D388"
FN_LINE = re.compile(r"@\s*(?:0x)?([0-9a-fA-F]{6,8})\b")
MAX_FUNCTIONS_HARD = 120
MAX_DEPTH_HARD = 2
MAX_DECOMPILE_HARD = 80


def extract_neighbors(observation):
    """Only trust canonical function-level listings ('... @ 0043d330')."""
    if not isinstance(observation, dict):
        return ()
    text = observation.get("text", "")
    if not isinstance(text, str):
        return ()
    found = []
    for value in FN_LINE.findall(text):
        try:
            address = bridge.validate_address("0x" + value)
        except bridge.ResearchError:
            continue
        if address != DISPUTED_ENTRY and address not in found:
            found.append(address)
    return tuple(found)


def safe_read(route, params, errors, address):
    """The underlying bridge allows only explicit GET routes on loopback."""
    try:
        return bridge.local_get(route, params)
    except bridge.ResearchError as exc:
        errors.append({"address": address, "route": route, "error": str(exc)})
        return {"error": str(exc)}


def collect(exe, max_functions=60, depth=1, max_decompile=28,
            progress=print, requester=None):
    """Return a self-contained manifest and JSON objects. No writes until ZIP.

    requester is injectable for fully offline unit tests.
    """
    if requester is None:
        requester = bridge.local_get
    if not len(SEEDS) <= max_functions <= MAX_FUNCTIONS_HARD:
        raise bridge.ResearchError(
            "max-functions must be %d..%d" % (len(SEEDS), MAX_FUNCTIONS_HARD))
    if not 0 <= depth <= MAX_DEPTH_HARD:
        raise bridge.ResearchError("depth must be 0..2")
    if not 0 <= max_decompile <= MAX_DECOMPILE_HARD:
        raise bridge.ResearchError("max-decompile must be 0..80")
    if max_decompile > max_functions:
        raise bridge.ResearchError("max-decompile cannot exceed max-functions")

    # Fail closed BEFORE code queries; current bridge still cannot prove that
    # Ghidra's imported program is byte-identical to the verified disk file.
    disk = bridge.check_executable(Path(exe))
    metadata = requester("/get_metadata", {"program": "speed.exe"})
    if not bridge.expected_program_metadata(metadata):
        raise bridge.ResearchError("Ghidra did not report speed.exe")
    health = requester("/mcp/health")

    pending = deque((seed, 0) for seed in SEEDS)
    queued = set(SEEDS)
    reports = {}
    errors = []
    decompiled = 0

    while pending and len(reports) < max_functions:
        address, distance = pending.popleft()
        progress("[%d/%d] %s (depth %d)" %
                 (len(reports) + 1, max_functions, address, distance))
        params = {"address": address, "limit": 60, "program": "speed.exe"}
        observations = {}
        for name, route in (
            ("function_callers", "/get_function_callers"),
            ("function_callees", "/get_function_callees"),
            ("xrefs_to", "/get_xrefs_to"),
        ):
            try:
                observations[name] = requester(route, params)
            except (bridge.ResearchError, OSError, ValueError) as exc:
                msg = str(exc)
                errors.append({"address": address, "route": route, "error": msg})
                observations[name] = {"error": msg}

        if decompiled < max_decompile:
            route = "/decompile_function"
            try:
                observations["decompilation"] = requester(
                    route, {"address": address, "program": "speed.exe", "timeout": 30})
                decompiled += 1
            except (bridge.ResearchError, OSError, ValueError) as exc:
                msg = str(exc)
                errors.append({"address": address, "route": route, "error": msg})
                observations["decompilation"] = {"error": msg}

        reports[address] = {
            "schema": "frr-ghidra-bulk-v1",
            "address": address,
            "graph_depth": distance,
            "observations": observations,
        }
        if distance < depth:
            for key in ("function_callers", "function_callees"):
                for neighbor in extract_neighbors(observations.get(key)):
                    if neighbor not in queued:
                        queued.add(neighbor)
                        pending.append((neighbor, distance + 1))

    manifest = {
        "schema": "frr-ghidra-bulk-v1",
        "source": "local-only Ghidra MCP read-only GET endpoints",
        "collected_at_utc": datetime.now(timezone.utc).isoformat(),
        "disk_executable": disk,
        "ghidra_metadata": metadata,
        "ghidra_health": health,
        "ghidra_program_bytes_verified": False,
        "note": ("Disk MD5 checked. Ghidra program recognized by name/metadata, "
                 "but imported bytes have NOT been independently compared. "
                 "All function identities and ABIs remain research leads."),
        "limits": {
            "max_functions": max_functions,
            "depth": depth,
            "max_decompile": max_decompile,
        },
        "count": len(reports),
        "decompiled": decompiled,
        "errors": errors,
        "seed_addresses": SEEDS,
        "unvisited_pending": len(pending),
    }
    return manifest, reports


def make_zip(manifest, reports, destination):
    """Never overwrite an existing file and never place EXE bytes in ZIP."""
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, "x", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=6) as zf:
        zf.writestr("manifest.json", json.dumps(
            manifest, ensure_ascii=False, indent=2) + "\n")
        for address, report in reports.items():
            zf.writestr("functions/" + address + ".json", json.dumps(
                report, ensure_ascii=False, indent=2) + "\n")
        zf.writestr("LEIA-ME.txt",
                    "FreeRoamRivals: pesquisa Ghidra, somente leitura.\n"
                    "Envie este ZIP ao chat para analise; nao publique pseudocodigo "
                    "ou binario do jogo no GitHub.\n"
                    "get_metadata nao comprova identidade byte-a-byte do programa "
                    "importado no Ghidra.\n"
                    "Os dados nao autorizam hooks ou alteracoes no jogo.\n")
    return destination


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--max-functions", type=int, default=60,
                        help="max 120, minimum the number of curated seeds")
    parser.add_argument("--depth", type=int, default=1, help="graph hops: 0..2")
    parser.add_argument("--max-decompile", type=int, default=28,
                        help="how many results include pseudocode: 0..80")
    args = parser.parse_args(argv)
    try:
        manifest, reports = collect(args.exe, args.max_functions,
                                    args.depth, args.max_decompile)
        stamp = datetime.now().strftime("%Y%m%d-%H%M%S-%f")
        path = bridge.LOCAL_REPORTS / ("ghidra_bulk_" + stamp + ".zip")
        make_zip(manifest, reports, path)
    except (bridge.ResearchError, FileExistsError, OSError, ValueError) as exc:
        parser.error(str(exc))
    print("\nDONE: " + str(path))
    print("Functions: %d; decompiled: %d; errors: %d" %
          (manifest["count"], manifest["decompiled"], len(manifest["errors"])))
    print("Share just this ZIP here. Ghidra, game EXE and ASI were not changed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
