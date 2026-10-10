# Saved checkpoint — v0.0.41 real game test, 2026-10-10

User instruction: save EVERYTHING only; do not continue development, research, fixes, builds or tests.

## User report

“Agora funcionou, ele andou e etc... porém sumiu”. The user observed the GTI driving and subsequently disappearing. This is the first reported successful driving test of v0.0.41. The cause of disappearance is not established here. No fix is attempted.

## Current project snapshot

- Repository: https://github.com/Rogeriomatador/Free-Roam-Rivals-Mod-NFSMW
- Working release: v0.0.41-dev
- Code commit: 044178ce4f6ec2cf3010a2b7921d4f04395d0f93
- Code tree: d4ced3c0173a200a2b06df91a5a8e68d187ffcb3
- Merged PR: #53.
- Prior completed verification: 20 CTest targets and 26 Python tests pass in Windows. No new tests run for this save-only request.
- Release: https://github.com/Rogeriomatador/Free-Roam-Rivals-Mod-NFSMW/releases/tag/v0.0.41-dev
- Enabled user INI remains the prior configuration; no new INI was supplied in this upload and no settings are changed.

## Evidence retained in full

| File | Bytes | SHA-256 |
| --- | --- | --- |
| FreeRoamRivals(20261010-063114).log | 575653 | 02cc81f3a316ac0679f56238d92226f8337cf3b491ed5b5ea2afd2d977cc676c |
| NativeExceptions(2).log | 98304 | 01d3d3dee1819789976011f66131d7a9e111ad14f504491109accf399add866b |

Files are saved byte-for-byte, including original line endings and the complete bounded exception trace. They contain diagnostic metadata, not game executable bytes or savegames.

## Capture landmarks for resuming later

- 03:30:07.932: constructor returns Golf GTI, handle=614, synchronousBaselinePreserved=1.
- 03:30:07.954: identity confirmed in two completed gameplay frames.
- 03:30:08.460: Racer driver/goal preparation reaches preparing_road.
- 03:30:08.703: road reset reaches activating.
- 03:30:08.959: reaches active after SetSpawned, native Racer goal and Activate.
- Active observations show changing trajectory and increasing speed; at 03:30:24.208 speedMps=46.8413, displacementMeters=448.166. Last active observation at 03:30:35.201 reports displacementMeters=837.735.
- 03:30:36.015: log records an F8 cleanup request, followed by retiring. This is recorded evidence; it is not proof of when or why the user-visible disappearance happened.
- Cleanup remains deferred for hidden/300m conditions; at 03:30:48.450 the log reports world_transition_no_old_pointer_reads.
- No explicit completed-removal or native-engine-retirement event was found in this capture. NativeExceptions contains zero FIRST_CHANCE records for the selected exception types.

## Paused state

Creation, confirmation, Racer preparation, road reset, activation and movement are now supported by this capture plus the user's observation. Persistence/disappearance remains unresolved. Do not infer that pursuit behavior, challenges, reward transactions or safe retirement are validated. The project is paused at the user's request. Resume only when requested, using these exact files and the pinned code commit; do not start from scratch.

The four last user-supplied INIs are also archived under `config-last-supplied/`. Their source filenames and hashes are recorded in `manifest.json`; they are not represented as newly uploaded files in this test. All 199 remote project files were checked against the workspace: no locally present file differs, and the two documents present only remotely remain saved in the pinned code tree.
