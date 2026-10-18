#!/usr/bin/env python3
"""Resolve suspected native call boundaries using local read-only Ghidra MCP.

One invocation collects documented GET results into one private ZIP.
This is a research-only audit: it cannot validate ABI, ownership or hooks.
Never uploads bytes, patches Ghidra or touches the game/save/ASI.
"""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import sys
import zipfile

try:
    from tools import ghidra_research_bridge as bridge
except ModuleNotFoundError:
    import ghidra_research_bridge as bridge

# These historical SDK/FRR leads were NOT recognized as callable function
# entries by the user's initial Ghidra bulk capture (2026-10-18).
# The 0x43D388 entry specifically conflicts with the enclosing constructor.
TARGETS = (
    ("0x00689820", "PVehicle Construct or ResetCar lead"),
    ("0x006851D0", "KillSimable lead; never call without verified ABI"),
    ("0x00688120", "offscreen-time getter lead"),
    ("0x006693A0", "IVehicle Activate lead"),
    ("0x006693C0", "IVehicle Deactivate lead"),
    ("0x006876E0", "SetDriverClass lead"),
    ("0x0043D388", "disputed interior address; NEVER assume entry"),
    ("0x0043D330", "positive control: goal-related initializer"),
    ("0x0043D400", "positive control: goal allocator"),
    ("0x0043EF70", "positive control: racer host initializer"),
    ("0x0043F040", "positive control: racer host allocator"),
    ("0x00422480", "positive control: goal setter"),
)
READ_ROUTES = (
    ("function_info", "/get_function_by_address"),
    ("disassembly", "/disassemble_function"),
    ("xrefs_to", "/get_xrefs_to"),
    ("memory", "/read_memory"),
)


def embedded_error(value):
    if not isinstance(value, dict):
        return "invalid response shape"
    if value.get("error"):
        return str(value["error"])
    msg = value.get("text")
    if isinstance(msg, str) and (
        "not found" in msg.lower()
        or "no function" in msg.lower()
        or "unknown endpoint" in msg.lower()
    ):
        return msg
    return None


def capture(exe, requester=None, progress=print):
    if requester is None:
        requester = bridge.local_get
    # Every target is bounded and validated, before any HTTP call.
    for address, label in TARGETS:
        bridge.validate_address(address)
    disk = bridge.check_executable(Path(exe))
    metadata = requester("/get_metadata", {"program": "speed.exe"})
    if not bridge.expected_program_metadata(metadata):
        raise bridge.ResearchError("Ghidra did not report speed.exe")
    health = requester("/mcp/health")
    results, errors = {}, []
    for index, (addr, label) in enumerate(TARGETS, 1):
        progress("[%d/%d] %s: %s" % (index, len(TARGETS), addr, label))
        observations = {}
        for key, route in READ_ROUTES:
            params = {"address": addr, "program": "speed.exe"}
            if key == "memory":
                params["length"] = 32
            if key == "xrefs_to":
                params["limit"] = 40
            try:
                response = requester(route, params)
                observations[key] = response
                msg = embedded_error(response)
                if msg:
                    errors.append({"address": addr, "route": route, "error": msg})
            except (bridge.ResearchError, OSError, ValueError) as exc:
                msg = str(exc)
                observations[key] = {"error": msg}
                errors.append({"address": addr, "route": route, "error": msg})
        results[addr] = {
            "address": addr,
            "lead": label,
            "research_only": True,
            "observations": observations,
        }
    manifest = {
        "schema": "frr-ghidra-native-boundary-v1",
        "at_utc": datetime.now(timezone.utc).isoformat(),
        "source": "local Ghidra MCP GET-only",
        "disk_executable": disk,
        "ghidra_metadata": metadata,
        "ghidra_health": health,
        "ghidra_program_bytes_verified": False,
        "note": ("The MD5 applies to the on-disk EXE only. "
                 "Neither Ghidra-imported bytes nor native ABI are proven. "
                 "Read responses (including disassembly) are research leads."),
        "targets": len(TARGETS),
        "errors": errors,
        "gating": "NO game hooks or native calls approved by this audit",
    }
    return manifest, results


def save_archive(manifest, results, path):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(path, "x", compression=zipfile.ZIP_DEFLATED) as archive:
        archive.writestr("manifest.json", json.dumps(
            manifest, ensure_ascii=False, indent=2) + "\n")
        for address, result in results.items():
            archive.writestr("addresses/" + address + ".json", json.dumps(
                result, ensure_ascii=False, indent=2) + "\n")
        archive.writestr("LEIA-ME.txt",
                         "Privado: nao publique estes dados no GitHub.\n"
                         "Leituras de Ghidra nao autorizam patch ou chamadas nativas.\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--exe", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        manifest, results = capture(args.exe)
        stamp = datetime.now().strftime("%Y%m%d-%H%M%S-%f")
        out = bridge.LOCAL_REPORTS / ("ghidra_native_boundaries_" + stamp + ".zip")
        save_archive(manifest, results, out)
    except (OSError, FileExistsError, ValueError, bridge.ResearchError) as exc:
        parser.error(str(exc))
    print("\nDONE: " + str(out))
    print("Addresses=%d, endpoint errors=%d; no native ABI verified." %
          (manifest["targets"], len(manifest["errors"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
