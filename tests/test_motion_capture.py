import importlib.util
from pathlib import Path
import unittest

spec = importlib.util.spec_from_file_location("audit", Path(__file__).resolve().parents[1] / "tools/analyze_motion_capture.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def records(count=14, model=1, cohort=1):
    rows = []
    for i in range(count):
        row = dict(capture=100, cohort=cohort, generation=1, model=model,
                   timeMs=1000 + i * 1000, dt=0 if i == 0 else 1,
                   valid=1, safe=1, grounded=1, accepted=0 if i == 0 else 1,
                   window=min(i, 120), stable=int(i >= 12), metricVerified=0,
                   x=i * 10, y=0, z=0, speed=10, speedometer=10, absolute=10,
                   local=10, linear=10, vx=10, vy=0, vz=0, slip=0,
                   ratio=0 if i == 0 else 1, cv=0)
        rows.append(row)
    return rows


def lines(rows):
    return ["[2026-10-07] [INFO] FRR_MOTION_V1 " + " ".join(f"{k}={v}" for k, v in row.items()) for row in rows]


class MotionCaptureTests(unittest.TestCase):
    def test_reproduce_window_and_raw_cross_checks(self):
        report = audit.analyze(lines(records(135)))
        cohort = report["cohorts"][0]
        self.assertEqual(cohort["acceptedPairs"], 134)
        self.assertEqual(cohort["lastWindowSamples"], 120)
        self.assertEqual(cohort["auditMismatches"], 0)
        self.assertEqual(cohort["directionWarnings"], 0)
        self.assertEqual(cohort["lastWindowMeans"]["worldUnitsPerSpeedUnitSecond"], 1)
        self.assertFalse(report["metricVerified"])

    def test_model_and_context_never_mix(self):
        report = audit.analyze(lines(records(model=10) + records(model=20, cohort=2)))
        self.assertEqual(len(report["cohorts"]), 2)
        self.assertTrue(all(c["acceptedPairs"] == 13 for c in report["cohorts"]))

    def test_malformed_nonfinite_duplicate_and_missing(self):
        rows = lines(records(2))
        for bad in (rows[1].replace("speed=10", "speed=nan"),
                    rows[1] + " speed=10", rows[1].replace(" linear=10", "")):
            report = audit.analyze([rows[0], bad])
            self.assertEqual(report["malformedRecords"], 1)
            self.assertEqual(report["cohorts"][0]["acceptedPairs"], 0)

    def test_gaps_cannot_be_bridged(self):
        rows = lines(records(3))
        report = audit.analyze([rows[0], "FRR_MOTION_V1 truncated", rows[2]])
        self.assertGreater(report["cohorts"][0]["auditMismatches"], 0)
        self.assertEqual(report["cohorts"][0]["acceptedPairs"], 0)

    def test_invalid_endpoint_clears_stability(self):
        rows = records()
        rows[-1].update(grounded=0, accepted=0, window=0, stable=0, ratio=0)
        report = audit.analyze(lines(rows))
        self.assertEqual(report["cohorts"][0]["auditMismatches"], 0)
        self.assertEqual(report["cohorts"][0]["lastWindowSamples"], 0)

    def test_tampered_timing_and_promotion_are_reported(self):
        rows = records()
        rows[-1].update(ratio=2, timeMs=500, metricVerified=1)
        report = audit.analyze(lines(rows))
        self.assertGreaterEqual(report["cohorts"][0]["auditMismatches"], 2)
        self.assertFalse(report["metricVerified"])

    def test_turning_is_flagged_without_discarding_scale(self):
        rows = records(2)
        rows[-1].update(vx=0, vz=10)
        report = audit.analyze(lines(rows))
        self.assertEqual(report["cohorts"][0]["directionWarnings"], 1)
        self.assertEqual(report["cohorts"][0]["acceptedPairs"], 1)

    def test_no_capture_is_not_a_valid_report(self):
        report = audit.analyze(["ordinary heartbeat"])
        self.assertEqual(report["cohorts"], [])
        self.assertFalse(report["metricVerified"])


if __name__ == "__main__":
    unittest.main()
