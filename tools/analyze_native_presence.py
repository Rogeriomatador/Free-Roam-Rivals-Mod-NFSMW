#!/usr/bin/env python3
"""Offline, read-only correlation of FreeRoamRivals.log native-presence incidents.

Does not execute the game, inject a DLL, change INIs or infer why a handle changed.
Use a log captured from the *instrumented* branch for the new handle fields.
"""
import argparse
import json
import re
from pathlib import Path

MAX_INPUT_BYTES = 32 * 1024 * 1024
STAMP = re.compile(r"^\[(?P<time>\d{4}-\d\d-\d\d \d\d:\d\d:\d\d\.\d{3})\] \[(?P<level>[A-Z]+)\] (?P<message>.*)$")
FIELD = re.compile(r"(?<!\S)([A-Za-z][A-Za-z0-9]*)=([^\s]+)")
INCIDENT = "NativePrototype sudden native presence loss:"

def _number(value):
    try:
        return int(value, 0) if value is not None and value != "not_read" else None
    except ValueError:
        # C++ outputs decimal integers without a prefix; 0-prefixed decimal
        # values do not necessarily use Python's base-0 convention.
        try:
            return int(value, 10)
        except (ValueError, TypeError):
            return None

def classify(fields):
    """Classification of *logged evidence*, NOT the cause of the engine event."""
    if fields.get("ownerReadReason") != "simable_handle":
        return "other_unavailability"
    if "handleMismatchObserved" not in fields:
        return "legacy_log_no_handle_fields"
    if fields.get("handleMismatchObserved") != "1":
        return "handle_comparison_unconfirmed"
    old = _number(fields.get("expectedSimableHandle"))
    new = _number(fields.get("observedSimableHandle"))
    if old is None or new is None or old == new:
        return "inconsistent_handle_evidence"
    if (fields.get("sameWorldContext") != "1" or
        fields.get("registryVerified") != "1" or
        fields.get("liveRegistryContainsOwner") != "1" or
        fields.get("physicalRegistryContainsOwner") != "1"):
        return "incomplete_registry_evidence"
    return "observed_handle_mismatch"

def analyze(lines):
    events = []
    incidents = []
    triggers = (
        ("constructor_requested", "NativeFactory constructor invoke:"),
        ("construction_confirmed", "NativeFactory construction confirmed:"),
        ("activation_requested", "NativePrototype activation begin:"),
        ("identity_restored", "NativePrototype physical snapshot became available again"),
        ("external_retirement", "NativePrototype native engine retired the vehicle"),
    )
    for line_number, raw_line in enumerate(lines, start=1):
        parsed = STAMP.match(raw_line.rstrip("\r\n"))
        if not parsed:
            continue
        message = parsed.group("message")
        kind = next((kind for kind, marker in triggers if marker in message), None)
        if INCIDENT in message:
            kind = "native_presence_loss"
        if kind is None:
            continue
        event = {"line": line_number, "time": parsed.group("time"), "kind": kind}
        events.append(event)
        if kind == "native_presence_loss":
            fields = dict(FIELD.findall(message.partition(INCIDENT)[2]))
            incidents.append({
                **event,
                "classification": classify(fields),
                "observed": {
                    key: fields.get(key) for key in (
                        "ownerReadReason", "sameWorldContext", "registryVerified",
                        "liveRegistryContainsOwner", "physicalRegistryContainsOwner",
                        "handleMismatchObserved", "expectedSimableHandle",
                        "observedSimableHandle", "lastReliableDistanceMeters",
                        "lastSampleAgeMs", "causeNotEstablished", "unsafeToForceRespawn")
                },
            })
    return {
        "schema": "frr-native-presence-v1",
        "read_only": True,
        "engine_cause_proven": False,
        "events": events,
        "incidents": incidents,
    }

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("log", type=Path, help="Path to FreeRoamRivals.log (not a game binary)")
    parser.add_argument("-o", "--output", type=Path, help="Write a new JSON report (never overwrites)")
    args = parser.parse_args(argv)
    if not args.log.is_file():
        parser.error("log not found")
    if args.log.stat().st_size > MAX_INPUT_BYTES:
        parser.error("log exceeds 32 MiB; use the diagnostic collector's bounded log")
    with args.log.open("r", encoding="utf-8", errors="replace") as stream:
        report = analyze(stream)
    report["source_filename"] = args.log.name
    serialized = json.dumps(report, ensure_ascii=False, indent=2) + "\n"
    if args.output:
        if args.log.resolve() == args.output.resolve():
            parser.error("cannot write output over the log")
        try:
            with args.output.open("x", encoding="utf-8") as stream:
                stream.write(serialized)
        except FileExistsError:
            parser.error("output already exists (will not overwrite)")
        print(f"Wrote {args.output}")
    else:
        print(serialized, end="")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
