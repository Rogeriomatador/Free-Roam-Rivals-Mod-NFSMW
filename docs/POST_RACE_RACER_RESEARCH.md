# Native post-race racers: research and v0.0.30 diagnostic

Research date: 2026-10-07. The owner reports that opponents sometimes remain
followable after races. This is an owner report, not a capture made here.

## Sources actually inspected

| Source / revision | Relevant findings | Evidence limit |
| --- | --- | --- |
| [dbalatoni13/nfsmw](https://github.com/dbalatoni13/nfsmw/tree/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c), `src/Speed/Indep/Src/AI/Common/AIVehicleRacecar.cpp` | Reconstructed constructor, StartRace, QuitRace and Update | Multi-platform WIP reconstruction; no local binary match against our pinned PC target |
| Same revision, `src/Speed/Indep/Src/AI/Common/AIVehicle.cpp` | UnSpawn clears the goal and deactivates the vehicle | Not destruction; does not prove a safe owned-racer UnSpawn/Kill sequence |
| Same revision, `src/Speed/Indep/Src/Physics/PVehicle.h` | VehicleParams field order, direction/position references, cache and flags | Supports SDK semantics; not an executed x86 ABI test |
| [s-b-repo/nfsmw-2005-re](https://github.com/s-b-repo/nfsmw-2005-re/tree/0dbc9393e263d95d182e6c98bff3dab4140aa2ff), `notes/project_ai_racer.md`, `docs/renames.csv` | Host/goal layers, racing-line and catchup dependencies | Notes and CSV disagree on an entry address; addresses need executable verification |
| [NFS Chat Chaos Mod](https://github.com/berkayylmao/NFS-Chat-Chaos-Mod/tree/2059ff1b71144328f50cbff18e05f50a063770af), `GuessWhosBack.hpp`, `Impostor.hpp` | Published construction, ground placement, cop AI, Kill and driver-class-change code | Source inspection, not local gameplay; swap effect operates within a stock race |
| [MW Native Free Roam Racer](https://github.com/Zakkey250/MW-NativeFreeRoamRacer/tree/c8e728809bdc77bfd11e31d975bb2bc7e0cce2bd), Runtime.cpp, AnchorRoadSeed.inl, VALIDATION.md | Published Traffic-seed to Racer activation and author-reported gameplay sessions | Different LAA executable hash, not executed here; its original code is not copied/bundled |
| [veritr1x/nfsmw-recomp](https://github.com/veritr1x/nfsmw-recomp), README.md, docs/testing.md | PC translation and test infrastructure | Requires original game files for gameplay; reports race-loading hangs and unrechecked platforms |

## What the owner's clue suggests

The reconstructed `AIVehicleRacecar::QuitRace()` clears the chase target,
restores input/collisions, switches navigation to direction/racing-lane mode,
disables the race filter, cancels pathfinding and resets road navigation.
It contains no Kill or UnSpawn call. Update continues updating a present goal.

Inference: that method can detach AI from a fixed race route without itself
destroying the vehicle. This is a plausible mechanism behind the report, not
proof of which race-end path calls it or how long GRaceStatus retains a car.
The mod must not adopt or destroy a stock opponent based on this inference.

NFRR provides another concrete lead: it validates a GRaceStatus cache interface,
constructs a Traffic seed, activates it, changes driver class, validates the
replaced AI interface, resets navigation from owned anchor values and ensures a
native racer goal/action exists. Its comments report that a driver-class change
alone may leave a null goal. A Racer enum is therefore insufficient evidence of
a functioning roaming AI. These are source findings, not copied implementation
or completed compatibility tests. The NFRR author permits research but restricts
redistribution of original code/modified builds; only links and findings belong
in this repository.

## Implemented diagnostic

`PostRaceRacerDiagnosticsEnabled=1` opts into snapshots every 500 ms after the
verified main loop completes on its consistent thread. No render probe or
FrameTick setting is needed for this diagnostic.

- Read the live IVehicle registry with guarded reads and a 512-slot ceiling.
- Record numeric IVehicle/ISimable/model identities, driver class, position,
  AI interface identity and interface vtable. No concrete AI cast or goal call.
- Capture Racer-class identities during Racing; correlate matching identities
  during Roaming only across consecutive complete samples in the same player,
  profile, race-status and road-network context.
- Measure displacement only between Roaming samples, excluding the initial
  race-to-roam interval where teleportation is possible.
- Revoke matches on absence, incomplete reads, invalid/duplicate identity,
  context changes, clock reversal, gaps over two seconds, loading/NIS/fade,
  or expiry two minutes after the last Racing sample.

Every match says `lifetimeProven=0`. Addresses may be recycled between samples;
displacement can also be caused by a teleport or collision. Zero matches is
inconclusive: fade/loading intentionally discards the cohort, driver classes
can change, and cars may disappear between samples. No observation promotes a
construction/AI readiness gate. No stock car is controlled or renamed.

When back at the PC: enable the setting, complete a normal career race, return
to Free Roam, follow any remaining opponent for 30-60 seconds and capture the
log. No new capture was made or requested to finish the offline work.

## Validation boundary

Portable tests use synthetic value snapshots to check correlations, movement
intervals, disappearance/address reuse, context changes, incomplete reads,
duplicate identities, gaps and expiry. They are not a game simulation. The
Windows build compiles the real probe but does not run it in speed.exe. This
revision does not create a native rival.
