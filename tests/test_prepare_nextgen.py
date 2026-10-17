"""Offline release-pipeline regression tests: no speed.exe, game session or network."""
import hashlib
import pathlib
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from tools import prepare_nextgen as nxt


def ini_fixture(newline="\r\n"):
    groups = {}
    for (section, key), default in nxt.PRESET.items():
        groups.setdefault(section, []).append(f"{key} = {default}")
    # Real upstream INI contains comments and CRLF. Override a few defaults
    # to prove the builder, not the fixture, applies the curated safe preset.
    groups["Encounter"].append("; owner=upstream")
    groups["Population"] = [x.replace("MaximumRacers = 4", "MaximumRacers = 15") for x in groups["Population"]]
    groups["Updates"] = [x.replace("Enabled = 0", "Enabled = 1") for x in groups["Updates"]]
    return newline.join("[" + sec + "]" + newline + newline.join(lines)
                        for sec, lines in groups.items()) + newline


class NextgenCandidateTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)
        self.asset = self.root / nxt.UPSTREAM_ASSET
        self.manifest = self.root / "SHA256SUMS.txt"
        self.license = self.root / "LICENSE.md"
        self.license.write_text("MIT License\nCopyright (c) 2026 Zakkey250\n", encoding="utf-8")
        self.ini = ini_fixture()

    def pack(self, *, extra=None, include_tpk=True):
        entries = {
            "release/scripts/NFSMWNativeFreeRoamRacers.asi": b"MZ" + b"x" * 256,
            "release/scripts/NFSMWNativeFreeRoamRacers.ini": self.ini.encode("utf-8"),
            "release/scripts/NFSMWNativeFreeRoamRacers.tpk": b"mock-tpk" * 16,
            "release/scripts/FreeRoamRivals.asi": b"foreign-mod-must-not-ship" * 16,
            "release/scripts/NFSMWLimitBreaker.asi": b"optional-must-not-ship" * 16,
        }
        if not include_tpk:
            del entries["release/scripts/NFSMWNativeFreeRoamRacers.tpk"]
        if extra:
            entries.update(extra)
        with zipfile.ZipFile(self.asset, "w", zipfile.ZIP_DEFLATED) as z:
            for name, data in entries.items():
                z.writestr(name, data)
        self.manifest.write_text(f"{nxt.sha256(self.asset)}  {self.asset.name}\n", encoding="utf-8")

    def stage(self):
        return nxt.create_candidate(self.asset, self.manifest, self.root / "stage", self.license)

    def test_standalone_only_three_native_runtime_files(self):
        self.pack()
        root = self.stage()
        installed = root / "scripts"
        self.assertEqual(
            set(x.name for x in installed.iterdir()),
            {"NFSMWNativeFreeRoamRacers.asi", "NFSMWNativeFreeRoamRacers.ini",
             "NFSMWNativeFreeRoamRacers.tpk"},
        )
        self.assertIn("MIT License", (root / "LICENCA-NFRR-MIT.txt").read_text())
        text = (installed / "NFSMWNativeFreeRoamRacers.ini").read_text()
        self.assertIn("MaximumRacers = 4", text)
        self.assertIn("Language = en", text)
        self.assertIn("AIMode = Custom AI", text)
        self.assertIn("[Updates]\nEnabled = 0", text)
        self.assertIn("NUNCA rode FreeRoamRivals.asi", (root / "LEIA-ME-ANTES-PTBR.txt").read_text())
        self.assertNotIn("FreeRoamRivals.asi", "\n".join(x.name for x in installed.iterdir()))
        self.assertIn("NFSMWNativeFreeRoamRacers.asi", (root / "HASHES-SHA256.txt").read_text())

    def test_crlf_preservation_and_no_truncated_lines(self):
        self.pack()
        root = self.stage()
        data = (root / "scripts" / "NFSMWNativeFreeRoamRacers.ini").read_bytes()
        self.assertIn(b"MaximumRacers = 4\r\n", data)
        self.assertIn(b"[Population]\r\n", data)
        self.assertIn(b"[Updates]\r\nEnabled = 0\r\n", data)

    def test_wrong_zip_hash_fails_without_staging(self):
        self.pack()
        self.manifest.write_text("0" * 64 + "  " + self.asset.name + "\n", encoding="utf-8")
        with self.assertRaisesRegex(nxt.NextError, "mismatch"):
            self.stage()
        self.assertFalse((self.root / "stage").exists())

    def test_missing_released_tpk_fails_before_staging(self):
        self.pack(include_tpk=False)
        with self.assertRaisesRegex(nxt.NextError, "tpk"):
            self.stage()
        self.assertFalse((self.root / "stage").exists())

    def test_zip_slip_and_symlink_rejected(self):
        self.pack(extra={"../evil/FreeRoamRivals.asi": b"x" * 60})
        with self.assertRaisesRegex(nxt.NextError, "unsafe archive entry"):
            self.stage()
        self.assertFalse((self.root / "stage").exists())

    def test_duplicate_same_named_asi_rejected(self):
        self.pack(extra={"different/NFSMWNativeFreeRoamRacers.asi": b"y" * 60})
        with self.assertRaisesRegex(nxt.NextError, "duplicate"):
            self.stage()

    def test_incompatible_speed_exe_blocks_candidate(self):
        self.pack()
        fake_exe = self.root / "speed.exe"
        fake_exe.write_bytes(b"MZ" + b"x" * 100)
        with self.assertRaisesRegex(nxt.NextError, "unsupported"):
            nxt.create_candidate(self.asset, self.manifest, self.root / "stage",
                                 self.license, fake_exe)
        self.assertFalse((self.root / "stage").exists())

    def test_never_overwrites_user_or_existing_stage(self):
        self.pack()
        root = self.stage()
        with self.assertRaisesRegex(nxt.NextError, "refusing to overwrite"):
            self.stage()
        self.assertEqual((root / "scripts" / "NFSMWNativeFreeRoamRacers.asi").read_bytes(),
                         b"MZ" + b"x" * 256)

    def test_missing_license_or_setting_fails(self):
        self.pack()
        self.license.unlink()
        with self.assertRaisesRegex(nxt.NextError, "license"):
            self.stage()
        self.license.write_text("MIT License\n", encoding="utf-8")
        self.ini = self.ini.replace("MaximumRacers = 15", "NotTheActualSetting = 1")
        self.pack()
        with self.assertRaisesRegex(nxt.NextError, "missing required preset"):
            self.stage()

    def test_checksum_text_must_pin_expected_asset(self):
        self.pack()
        self.manifest.write_text(nxt.sha256(self.asset) + "  unknown-release.zip\n", encoding="utf-8")
        with self.assertRaisesRegex(nxt.NextError, "does not identify"):
            self.stage()


if __name__ == "__main__":
    unittest.main()
