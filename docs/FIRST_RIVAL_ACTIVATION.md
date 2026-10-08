# One-rival pilot: exact-target activation evidence

Priority: issue #5. The next playable milestone is one independently created roaming rival, with no rewards, UI, Blacklist or economy changes. Post-race observation is not creation proof.

## Inspected reference

User supplied the NFRR v0.1.1 release package on 2026-10-08. ZIP CRC verification passed. It includes compiled ASIs, INIs, documentation and optional Limit Breaker API headers; it does not include the complete racer implementation source. The public source inspected separately is Zakkey250/MW-NativeFreeRoamRacer at c8e728809bdc77bfd11e31d975bb2bc7e0cce2bd. This is source evidence, not a claim that the shipped binary matches that commit.

NFRR README requires an English 1.3 LAA executable with SHA-256 B248271BF8EAC8C9B283B8C95E3ADD672B713BF529B05F1780E58268493B9D06. FRR uses size 6029312 and MD5 C0516B485065FABDD69579816B5DF763. Different hash algorithms cannot establish equality or difference. Compatibility must be checked against actual executable bytes.

## Useful findings, paraphrased

- Native creation uses a prepared vehicle record and moving traffic as a navigation anchor. It captures an owned road seed before allocation, because allocation can invalidate borrowed references.
- Construction protects existing live vehicles and rejects a returned pointer already belonging to that set.
- Activation is followed by Racer driver conversion, fresh AI validation, road initialization and spawned state. The source explicitly handles a missing racer goal and selects its native driving action. A valid AI pointer alone does not prove a driving rival.
- Reservation is one prepared construction slot, not the entire missing population. Reclamation and construction happen on separate ticks, after fresh vacancy and lifecycle checks. This avoids starving traffic anchors and colliding with deferred retirement.
- Existing AI continues independently of slower spawn management. Preparation and retry work are bounded.

## Concrete next verification

The existing read-only executable dump tool now also collects activation, deactivation, driver conversion, AI road reset, spawned state, SetGoal, ChooseAction, retirement and off-screen-time candidates. Addresses are explicitly foreign-target leads, never runtime authorization.

Run from the repository root:

```text
python tools/dump_exe_functions.py "PATH/TO/speed.exe" --bytes 4096 -o FRR_first_rival_activation.txt
```

Review the exact target factory argument layout, activation/goal ABI, native record lifetime, registry replacement behavior, cleanup identity and pursuit-state reader before connecting a one-rival adapter. No constructor, goal mutation or retirement call was enabled by this change. No game execution was performed. A raw executable or the generated report is still needed; previously supplied Claude conclusions lacked their original dumps.

## Rights

The supplied LICENSE.md permits research, discussion and personal local modifications, but prohibits redistribution of original code/assets or modified builds without express permission. This document records findings in our own words; no implementation code, ASI or assets were imported. Third-party libraries retain their own licenses.

## Acceptance

One fresh owned rival moves using native AI; survives a normal free-roam interval; respects pursuit, loading and world transitions; never replaces the player, traffic or stock opponents; cleanup verifies the exact owned identity and native retirement completion. Tests of a pure state machine are not this acceptance test.
