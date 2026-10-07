#!/usr/bin/env python3
"""Audit opt-in FRR_MOTION_V1 logs. Consistency never authorizes metric promotion."""
import argparse
from collections import deque
import json
import math
from pathlib import Path

MARKER = "FRR_MOTION_V1 "
INT_FIELDS = "capture cohort generation model timeMs valid safe grounded accepted window stable metricVerified".split()
FLOAT_FIELDS = "dt x y z speed speedometer absolute local linear vx vy vz slip ratio cv".split()


def parse_record(line):
    raw = line.split(MARKER, 1)[1].strip().split()
    fields = dict(token.split("=", 1) for token in raw)
    if len(fields) != len(raw):
        raise ValueError("duplicate field")
    result = {name: int(fields[name]) for name in INT_FIELDS}
    result.update({name: float(fields[name]) for name in FLOAT_FIELDS})
    if not all(math.isfinite(result[name]) for name in FLOAT_FIELDS):
        raise ValueError("nonfinite channel")
    if any(result[name] not in (0, 1) for name in
           "valid safe grounded accepted stable metricVerified".split()):
        raise ValueError("invalid boolean")
    if min(result[name] for name in "capture cohort generation model timeMs window".split()) < 0:
        raise ValueError("negative identifier/count")
    return result


def pair_values(previous, current):
    if previous is None:
        return None
    for row in (previous, current):
        if not all(row[name] for name in ("valid", "safe", "grounded")):
            return None
        if row["speed"] <= 0 or row["local"] <= .0001 or row["linear"] <= .0001:
            return None
    dt = current["dt"]
    if dt < .1 - 1e-7 or dt > 2.5 + 1e-7:
        return None
    speed = (previous["speed"] + current["speed"]) * .5
    if speed < 2 or abs(current["speed"] - previous["speed"]) / speed > .25 + 1e-7:
        return None
    distance = math.dist([previous[k] for k in ("x", "y", "z")],
                         [current[k] for k in ("x", "y", "z")])
    if distance < .05:
        return None
    average = lambda name: (abs(previous[name]) + abs(current[name])) * .5
    return (distance / (speed * dt), average("speedometer") / speed,
            average("absolute") / speed, speed / average("local"), speed / average("linear"))


def analyze(lines):
    groups = {}
    malformed = 0
    for line in lines:
        if MARKER not in line:
            continue
        try:
            row = parse_record(line)
        except (ValueError, KeyError, OverflowError):
            malformed += 1
            # Never bridge a missing/invalid sample, even if its cohort is unknown.
            for state in groups.values():
                state["previous"] = None
                state["window"].clear()
            continue
        key = tuple(row[k] for k in ("capture", "cohort", "generation", "model"))
        state = groups.setdefault(key, {"previous": None, "window": deque(maxlen=120),
                                      "samples": 0, "accepted": 0, "mismatches": 0,
                                      "directionWarnings": 0, "minRatio": None, "maxRatio": None})
        previous = state["previous"]
        values = pair_values(previous, row)
        state["samples"] += 1
        if bool(row["accepted"]) != (values is not None) or row["metricVerified"] != 0:
            state["mismatches"] += 1
        if previous is not None and (row["timeMs"] <= previous["timeMs"] or
                abs((row["timeMs"] - previous["timeMs"]) / 1000 - row["dt"]) > .05):
            state["mismatches"] += 1
        if row["accepted"] and values is not None:
            state["window"].append(values)
            state["accepted"] += 1
            ratio = values[0]
            state["minRatio"] = ratio if state["minRatio"] is None else min(state["minRatio"], ratio)
            state["maxRatio"] = ratio if state["maxRatio"] is None else max(state["maxRatio"], ratio)
            displacement = [row[k] - previous[k] for k in ("x", "y", "z")]
            for endpoint in (previous, row):
                velocity = [endpoint[k] for k in ("vx", "vy", "vz")]
                denominator = math.hypot(*displacement) * math.hypot(*velocity)
                alignment = sum(a * b for a, b in zip(displacement, velocity)) / denominator if denominator else 0
                if alignment < .995:
                    state["directionWarnings"] += 1
                    break
        else:
            state["window"].clear()
        window = state["window"]
        mean = sum(v[0] for v in window) / len(window) if window else 0
        variance = max(0, sum(v[0] * v[0] for v in window) / len(window) - mean * mean) if window else 0
        cv = math.sqrt(variance) / mean if mean else 0
        stable = len(window) >= 12 and cv <= .05
        if row["window"] != len(window) or not math.isclose(row["ratio"], mean, rel_tol=1e-4, abs_tol=1e-5) or \
                not math.isclose(row["cv"], cv, rel_tol=1e-3, abs_tol=1e-4) or bool(row["stable"]) != stable:
            state["mismatches"] += 1
        state["previous"] = row if row["valid"] else None
    reports = []
    for key, state in groups.items():
        window = state["window"]
        means = [sum(v[i] for v in window) / len(window) for i in range(5)] if window else [None] * 5
        reports.append(dict(zip(("capture", "cohort", "generation", "model"), key),
                            samples=state["samples"], acceptedPairs=state["accepted"],
                            auditMismatches=state["mismatches"], directionWarnings=state["directionWarnings"],
                            lastWindowSamples=len(window), minObservedRatio=state["minRatio"],
                            maxObservedRatio=state["maxRatio"],
                            lastWindowMeans=dict(zip(("worldUnitsPerSpeedUnitSecond", "speedometerToSpeed",
                                "absoluteToSpeed", "speedToLocal", "speedToLinear"), means))))
    return {"schema": "FRR_MOTION_AUDIT_V1", "metricVerified": False,
            "timeBasis": "wall_clock_not_proven_simulation_time", "malformedRecords": malformed,
            "cohorts": reports}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    with args.log.open(encoding="utf-8-sig", errors="replace") as handle:
        report = analyze(handle)
    print(json.dumps(report, indent=2, allow_nan=False))
    return 1 if not report["cohorts"] or report["malformedRecords"] or \
        any(c["auditMismatches"] for c in report["cohorts"]) else 0


if __name__ == "__main__":
    raise SystemExit(main())
