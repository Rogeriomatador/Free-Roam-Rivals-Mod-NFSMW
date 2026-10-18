"""Synthetic regressions for the read-only native presence log analyzer."""
import unittest

from tools.analyze_native_presence import analyze, classify

STAMP = "[2026-10-18 03:44:12.123] [WARN] "
BASE = ("sameWorldContext=1 ownerReadReason=simable_handle registryVerified=1 "
        "liveRegistryContainsOwner=1 physicalRegistryContainsOwner=1 ")

class NativePresenceTests(unittest.TestCase):
    def test_mismatch_with_both_memberships_is_observed_not_causally_explained(self):
        log = [
            "[2026-10-18 03:44:10.123] [INFO] NativeFactory construction confirmed: identity verified in two completed frames; inactive handle stamped\n",
            STAMP + "NativePrototype sudden native presence loss: " + BASE +
            "handleMismatchObserved=1 expectedSimableHandle=1024 observedSimableHandle=2048 "
            "causeNotEstablished=1 unsafeToForceRespawn=1\n",
        ]
        result = analyze(log)
        self.assertEqual(len(result["incidents"]), 1)
        self.assertEqual(result["incidents"][0]["classification"], "observed_handle_mismatch")
        self.assertFalse(result["engine_cause_proven"])
        self.assertEqual([e["kind"] for e in result["events"]],
                         ["construction_confirmed", "native_presence_loss"])

    def test_missing_new_fields_in_old_logs_stays_unproven(self):
        report = analyze([STAMP + "NativePrototype sudden native presence loss: " + BASE + "\n"])
        self.assertEqual(report["incidents"][0]["classification"], "legacy_log_no_handle_fields")

    def test_missing_registry_membership_prevents_confirmed_classification(self):
        fields = dict(ownerReadReason="simable_handle", handleMismatchObserved="1",
                      expectedSimableHandle="40", observedSimableHandle="41",
                      sameWorldContext="1", registryVerified="1",
                      liveRegistryContainsOwner="0", physicalRegistryContainsOwner="1")
        self.assertEqual(classify(fields), "incomplete_registry_evidence")

    def test_no_mismatch_or_unreadable_handles_are_not_mismatch_evidence(self):
        prefix = dict(ownerReadReason="simable_handle", handleMismatchObserved="1",
                      sameWorldContext="1", registryVerified="1",
                      liveRegistryContainsOwner="1", physicalRegistryContainsOwner="1")
        self.assertEqual(classify({**prefix, "expectedSimableHandle": "7",
                                   "observedSimableHandle": "7"}), "inconsistent_handle_evidence")
        self.assertEqual(classify({**prefix, "expectedSimableHandle": "not_read",
                                   "observedSimableHandle": "42"}), "inconsistent_handle_evidence")

    def test_empty_or_other_unavailability_does_not_fabricate_cause(self):
        self.assertEqual(analyze(["unstructured line"])["incidents"], [])
        self.assertEqual(classify({"ownerReadReason": "world_identity_changed"}),
                         "other_unavailability")

if __name__ == "__main__":
    unittest.main()
