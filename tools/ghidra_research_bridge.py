#!/usr/bin/env python3
"""FreeRoamRivals research bridge: local Ghidra MCP (read-only) + nfsmw-re CSV.

This is a LOCAL/offline development tool, never imported into the Win32 ASI.
No source or game binaries are uploaded. It never sends POST or changes Ghidra.
"""
import argparse
import csv
import hashlib
import json
import re
import sys
import urllib.parse
import urllib.request
from pathlib import Path

EXPECTED_EXE_BYTES = 6029312
EXPECTED_EXE_MD5 = "C0516B485065FABDD69579816B5DF763"
LOCAL_GHIDRA = "http://127.0.0.1:8089"
READ_ONLY_ROUTES = frozenset(("/mcp/health", "/get_metadata",
                              "/get_xrefs_to", "/get_xrefs_from",
                              "/get_function_callers", "/get_function_callees",
                              "/decompile_function"))
MAX_HTTP_BYTES = 2 * 1024 * 1024
MAX_CSV_BYTES = 8 * 1024 * 1024
ROOT = Path(__file__).resolve().parents[1]
LOCAL_REPORTS = ROOT / "research" / "local"
ADDRESS = re.compile(r"^0x[0-9a-fA-F]{6,8}$")


class ResearchError(Exception):
    pass


def check_executable(path):
    """Verify the user's *local* game exe; never open it as code."""
    if not path.is_file():
        raise ResearchError("speed.exe not found")
    if path.name.casefold() != "speed.exe":
        raise ResearchError("expected a file named speed.exe")
    if path.stat().st_size != EXPECTED_EXE_BYTES:
        raise ResearchError("wrong speed.exe size: fail closed, no Ghidra queries")
    digest = hashlib.md5()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    actual = digest.hexdigest().upper()
    if actual != EXPECTED_EXE_MD5:
        raise ResearchError("wrong speed.exe MD5: " + actual + "; fail closed")
    return {"size": EXPECTED_EXE_BYTES, "md5": actual, "supported": True}


def validate_address(s):
    if not ADDRESS.fullmatch(s):
        raise ResearchError("address must be hexadecimal, e.g. 0x0043EF70")
    value = int(s, 16)
    if not 0x00400000 <= value < 0x00C00000:
        raise ResearchError("address outside conservative retail image range")
    return "0x%08X" % value


def parse_reference_csv(path, phrase, limit=20):
    """nfsmw-re names are research hints; no address is promoted to verified."""
    if not path.is_file() or path.stat().st_size > MAX_CSV_BYTES:
        raise ResearchError("reference CSV absent or over 8 MiB")
    results = []
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        reader = csv.DictReader(stream)
        if not {"address", "name", "xref_count"}.issubset(reader.fieldnames or []):
            raise ResearchError("reference CSV columns changed")
        for row in reader:
            if phrase.casefold() not in row["name"].casefold():
                continue
            try:
                addr = validate_address("0x" + row["address"].removeprefix("0x"))
                count = int(row["xref_count"])
            except (ValueError, ResearchError):
                continue
            results.append({"address": addr, "name": row["name"],
                            "xref_count": count, "status": "external_lead_only"})
    results.sort(key=lambda r: (-r["xref_count"], r["address"]))
    return results[:limit]


def local_get(route, params=None, opener=None):
    """Allowlisted GET to Ghidra on localhost; explicitly disables proxies."""
    if route not in READ_ONLY_ROUTES:
        raise ResearchError("refused non-read-only Ghidra route")
    opener = opener if opener is not None else urllib.request.build_opener(
        urllib.request.ProxyHandler({}))
    query = urllib.parse.urlencode(params or {})
    url = LOCAL_GHIDRA + route + ("?" + query if query else "")
    request = urllib.request.Request(url, headers={"Accept": "application/json"},
                                     method="GET")
    try:
        with opener.open(request, timeout=15) as response:
            raw = response.read(MAX_HTTP_BYTES + 1)
    except (OSError, ValueError) as exc:
        raise ResearchError("local Ghidra MCP unavailable: " + str(exc)) from exc
    if len(raw) > MAX_HTTP_BYTES:
        raise ResearchError("Ghidra MCP response over 2 MiB")
    result = raw.decode("utf-8", errors="replace")
    try:
        return json.loads(result)
    except json.JSONDecodeError:
        return {"text": result}


def expected_program_metadata(metadata):
    """Only verifies Ghidra REPORTS a speed.exe program; not its binary hash."""
    encoded = json.dumps(metadata, ensure_ascii=False).casefold()
    return "speed.exe" in encoded


def take_snapshot(exe, address, *, decompile=False, callgraph=False, requester=local_get):
    """Collect a limited read-only report. Call only with owner-supplied exe."""
    verified = check_executable(exe)
    address = validate_address(address)
    health = requester("/mcp/health")
    metadata = requester("/get_metadata", {"program": "speed.exe"})
    if not expected_program_metadata(metadata):
        raise ResearchError("Ghidra does not report speed.exe; no xref/decomp queries")
    params = {"address": address, "limit": 40, "program": "speed.exe"}
    reads = {
        "xrefs_to": requester("/get_xrefs_to", params),
        "xrefs_from": requester("/get_xrefs_from", params),
    }
    # get_xrefs_from inspects just the EXACT entry instruction. An empty
    # result does NOT mean the rest of the function makes no calls.
    # Function-level caller/callee queries avoid that false inference.
    if callgraph:
        fn_params = {"address": address, "limit": 60, "program": "speed.exe"}
        reads["function_callers"] = requester("/get_function_callers", fn_params)
        reads["function_callees"] = requester("/get_function_callees", fn_params)
    if decompile:
        reads["decompilation"] = requester(
            "/decompile_function", {"address": address, "program": "speed.exe", "timeout": 30})
    return {
        "schema": "frr-ghidra-research-v1",
        "access": "local_read_only",
        "source": "local Ghidra MCP",
        "executable_on_disk": verified,
        "ghidra_program_metadata": metadata,
        "ghidra_program_bytes_verified": False,
        "note": ("The disk MD5 matches FRR; Ghidra's loaded program was only "
                 "identified by metadata, NOT proven byte-identical. "
                 "Addresses/ABI are research leads, not mutation authorization."),
        "address": address,
        "ghidra_health": health,
        "observations": reads,
    }


def new_local_report(filename, obj):
    LOCAL_REPORTS.mkdir(parents=True, exist_ok=True)
    target = LOCAL_REPORTS / filename
    if target.name != filename or "/" in filename or "\\" in filename:
        raise ResearchError("report filename must not contain path components")
    with target.open("x", encoding="utf-8") as stream:
        json.dump(obj, stream, ensure_ascii=False, indent=2)
        stream.write("\n")
    return target


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    verify = sub.add_parser("verify", help="verify local MD5-gated speed.exe")
    verify.add_argument("--exe", type=Path, required=True)
    names = sub.add_parser("lookup", help="search a local copy of nfsmw-re renames.csv")
    names.add_argument("--csv", type=Path, required=True)
    names.add_argument("--text", required=True)
    names.add_argument("--limit", type=int, default=20)
    snap = sub.add_parser("snapshot", help="read xrefs from LOCAL Ghidra MCP only")
    snap.add_argument("--exe", type=Path, required=True)
    snap.add_argument("--address", required=True)
    snap.add_argument("--decompile", action="store_true",
                      help="also store proprietary pseudocode LOCALLY; never commit report")
    snap.add_argument("--callgraph", action="store_true",
                      help="also query read-only, function-level callers and callees")
    snap.add_argument("--report", help="new filename under research/local (never overwrite)")
    args = parser.parse_args(argv)
    try:
        if args.command == "verify":
            result = check_executable(args.exe)
        elif args.command == "lookup":
            if not 1 <= args.limit <= 100:
                raise ResearchError("limit must be 1..100")
            result = {"reference": "nfsmw-re", "status": "unverified_external_leads",
                      "results": parse_reference_csv(args.csv, args.text, args.limit)}
        else:
            addr = validate_address(args.address)
            result = take_snapshot(args.exe, addr, decompile=args.decompile,
                                   callgraph=args.callgraph)
            filename = args.report or ("ghidra_" + addr[2:] + ".json")
            dest = new_local_report(filename, result)
            print("Local-only report: " + str(dest))
            return 0
    except (ResearchError, FileExistsError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, indent=2, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
