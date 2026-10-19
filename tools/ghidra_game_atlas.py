#!/usr/bin/env python3
"""Build a complete searchable Ghidra function ADDRESS INDEX (not game source).

Connects only to the already running loopback Ghidra MCP, using GET.
Reads 500 function records per request to avoid flooding Ghidra.
Exports names/addresses/thunk flags, NOT machine bytes or pseudocode.
The ZIP is LOCAL, ignored by Git, and must never be committed publicly.

"Complete" means all FUNCTION ENTRIES currently recognized by Ghidra,
not all code, object methods, game data, or the game's original source.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import json
from pathlib import Path
import re
import sys
import zipfile

try:
    from tools import ghidra_research_bridge as bridge
except ModuleNotFoundError:
    import ghidra_research_bridge as bridge

MAX_FUNCTIONS = 40000
MAX_PAGE_SIZE = 1000
MAX_PAGES = 100
MAX_CATALOG_BYTES = 12 * 1024 * 1024
NAMED_AUTO = re.compile(r"^(?:FUN_|LAB_|DAT_|thunk_FUN_|sub_)[0-9a-fA-F]+$")
REPORTED_COUNT = re.compile(r"Function Count:\\s*(\\d+)", re.I)
HEX_ADDR = re.compile(r"^(?:0x)?([0-9a-fA-F]{6,8})$")


def program_count(metadata):
    encoded = json.dumps(metadata, ensure_ascii=False)
    found = REPORTED_COUNT.search(encoded)
    return int(found.group(1)) if found else None


def normalize_function(item):
    """Preserve Ghidra address-space disambiguation; never infer an ABI."""
    if not isinstance(item, dict):
        raise bridge.ResearchError("catalog item is not an object")
    address = item.get("address")
    name = item.get("name")
    if not isinstance(address, str) or not address or len(address) > 128:
        raise bridge.ResearchError("malformed function address in Ghidra page")
    if not isinstance(name, str) or not name or len(name) > 256:
        raise bridge.ResearchError("malformed function name in Ghidra page")
    if type(item.get("isThunk")) is not bool or type(item.get("isExternal")) is not bool:
        raise bridge.ResearchError("Ghidra function flags not booleans")
    full = item.get("address_full")
    space = item.get("address_space")
    if full is not None and (not isinstance(full, str) or len(full) > 128):
        raise bridge.ResearchError("invalid Ghidra full address")
    if space is not None and (not isinstance(space, str) or len(space) > 128):
        raise bridge.ResearchError("invalid Ghidra address space")
    match = HEX_ADDR.fullmatch(address)
    if not item["isExternal"]:
        if match is None:
            raise bridge.ResearchError("unexpected unqualified internal address: " + address)
        normal_address = bridge.validate_address("0x" + match.group(1))
    else:
        # Ghidra EXTERNAL symbols may have non-game address spaces.
        normal_address = ("0x" + match.group(1).upper()) if match else address

    return {
        "address": normal_address,
        "address_full": full if full is not None else normal_address,
        "address_space": space if space is not None else "default",
        "name": name,
        "is_thunk": item["isThunk"],
        "is_external": item["isExternal"],
    }


def read_page(requester, offset, limit):
    raw = requester("/list_functions_enhanced", {
        "program": "speed.exe", "offset": offset, "limit": limit})
    if not isinstance(raw, dict):
        raise bridge.ResearchError("Ghidra catalog page is not JSON object")
    if raw.get("error"):
        raise bridge.ResearchError("Ghidra catalog error: " + str(raw["error"]))
    entries = raw.get("functions")
    count = raw.get("count")
    page_offset = raw.get("offset")
    if not isinstance(entries, list) or type(count) is not int or type(page_offset) is not int:
        raise bridge.ResearchError("Ghidra catalog page missing functions/count/offset")
    if count != len(entries) or page_offset != offset or count > limit:
        raise bridge.ResearchError("Ghidra catalog pagination mismatch")
    return [normalize_function(item) for item in entries]


def collect(exe, *, page_size=500, requester=None, progress=print):
    if not 50 <= page_size <= MAX_PAGE_SIZE:
        raise bridge.ResearchError("page-size must be 50..1000")
    if requester is None:
        requester = bridge.local_get
    verified = bridge.check_executable(Path(exe))
    metadata = requester("/get_metadata", {"program": "speed.exe"})
    if not bridge.expected_program_metadata(metadata):
        raise bridge.ResearchError("Ghidra has not confirmed speed.exe")
    total_expected = program_count(metadata)
    if total_expected is not None and not 0 < total_expected <= MAX_FUNCTIONS:
        raise bridge.ResearchError("Ghidra reports an implausible function count")
    health = requester("/mcp/health")
    functions, seen = [], set()
    for page_number in range(MAX_PAGES):
        offset = page_number * page_size
        progress("Cataloging Ghidra function entries: %d%s" %
                 (offset, " / " + str(total_expected) if total_expected else ""))
        page = read_page(requester, offset, page_size)
        if not page:
            break
        for row in page:
            key = (row["address_space"], row["address_full"])
            if key in seen:
                raise bridge.ResearchError(
                    "duplicate function address; analysis changed during pagination")
            seen.add(key)
            functions.append(row)
        if len(functions) > MAX_FUNCTIONS:
            raise bridge.ResearchError("catalog exceeds 40000 function entries")
        if len(page) < page_size:
            break
    else:
        raise bridge.ResearchError("catalog exceeds page cap: no complete claim")
    if not functions:
        raise bridge.ResearchError("Ghidra returned no function entries")
    if total_expected is not None and total_expected != len(functions):
        raise bridge.ResearchError(
            "incomplete/changed Ghidra analysis: expected %d, received %d" %
            (total_expected, len(functions)))
    if len(functions) > MAX_FUNCTIONS:
        raise bridge.ResearchError("catalog exceeds cap")
    categorized = Counter("external" if row["is_external"] else
                          "auto_named" if NAMED_AUTO.fullmatch(row["name"]) else
                          "named" for row in functions)
    catalog = "".join(json.dumps(row, ensure_ascii=False, separators=(",", ":")) + "\n"
                      for row in functions).encode("utf-8")
    if len(catalog) > MAX_CATALOG_BYTES:
        raise bridge.ResearchError("catalog would exceed 12 MiB")
    manifest = {
        "schema": "frr-nfsmw-ghidra-function-atlas-v1",
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "access": "loopback_Ghidra_MCP_GET_only",
        "program": "speed.exe",
        "executable_on_disk": verified,
        "ghidra_metadata": metadata,
        "ghidra_health": health,
        "ghidra_program_bytes_verified": False,
        "function_count": len(functions),
        "reported_function_count": total_expected,
        "counts": dict(categorized),
        "page_size": page_size,
        "pages_used": (len(functions) - 1) // page_size + 1,
        "contains_game_binary": False,
        "contains_game_decompilation": False,
        "limitations": [
            "Function catalog = entries currently recognized by Ghidra, not complete original game source.",
            "Indirect virtual calls, unrecognized functions, dynamic runtime code and asset formats are not fully indexed.",
            "The Ghidra-imported program's bytes have NOT been checked against the disk MD5.",
            "Addresses and names here alone do NOT authorize native hooks or calls.",
        ],
    }
    return manifest, catalog


def save_zip(destination, manifest, catalog):
    destination = Path(destination)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(destination, "x", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=6) as archive:
        archive.writestr("manifest.json", json.dumps(
            manifest, ensure_ascii=False, indent=2) + "\n")
        archive.writestr("functions.jsonl", catalog)
        archive.writestr("LEIA-ME.txt",
                         "Indice de FUNCOES reconhecidas pelo Ghidra, nao codigo-fonte da EA.\n"
                         "Nao publicar no GitHub. Enviar o ZIP a esta conversa.\n"
                         "Permite pesquisar enderecos e nomes; interpretar o codigo requer "
                         "capturas especificas de xrefs/decompilacao.\n")


def search_zip(path, phrase=None, address=None, limit=30):
    if not 1 <= limit <= 100:
        raise bridge.ResearchError("limit must be 1..100")
    if (phrase is None) == (address is None):
        raise bridge.ResearchError("specify exactly one of --text or --address")
    if address is not None:
        address = bridge.validate_address(address)
    if phrase is not None and (not phrase or len(phrase) > 100):
        raise bridge.ResearchError("search text must be 1..100 characters")
    with zipfile.ZipFile(path) as archive:
        manifest = json.loads(archive.read("manifest.json"))
        if manifest.get("schema") != "frr-nfsmw-ghidra-function-atlas-v1":
            raise bridge.ResearchError("not a supported atlas ZIP")
        if archive.getinfo("functions.jsonl").file_size > MAX_CATALOG_BYTES:
            raise bridge.ResearchError("atlas catalog is too large")
        results = []
        for line in archive.read("functions.jsonl").decode("utf-8").splitlines():
            row = json.loads(line)
            if (phrase is not None and phrase.casefold() in row["name"].casefold()
                    or address is not None and address == row["address"]):
                results.append(row)
                if len(results) == limit:
                    break
    return {"program": manifest.get("program"), "results": results,
            "limit": limit, "warning": "A matching name/address is not an ABI proof."}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    build = sub.add_parser("build", help="index ALL Ghidra-recognized function entries")
    build.add_argument("--exe", required=True, type=Path)
    build.add_argument("--page-size", type=int, default=500)
    search = sub.add_parser("search", help="search the saved atlas without Ghidra")
    search.add_argument("--zip", required=True, type=Path)
    choice = search.add_mutually_exclusive_group(required=True)
    choice.add_argument("--text")
    choice.add_argument("--address")
    search.add_argument("--limit", type=int, default=30)
    args = parser.parse_args(argv)
    try:
        if args.command == "build":
            manifest, catalog = collect(args.exe, page_size=args.page_size)
            stamp = datetime.now().strftime("%Y%m%d-%H%M%S-%f")
            out = bridge.LOCAL_REPORTS / ("ghidra_game_atlas_" + stamp + ".zip")
            save_zip(out, manifest, catalog)
            print("\nDONE: " + str(out))
            print("Ghidra function entries: %d; metadata bytes: %d" %
                  (manifest["function_count"], len(catalog)))
            print("Send ONLY the ZIP. This is an index; not all decompiled code.")
        else:
            print(json.dumps(search_zip(args.zip, args.text, args.address, args.limit),
                             indent=2, ensure_ascii=False))
    except (bridge.ResearchError, OSError, ValueError, KeyError, zipfile.BadZipFile) as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    sys.exit(main())
