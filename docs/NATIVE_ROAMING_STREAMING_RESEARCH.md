# Native roaming, streaming and vehicle-cache research

Date: 2026-10-07. This is offline source inspection, not a game run or a
binary/layout match against the owner's MD5-pinned speed.exe.

## Sources inspected and boundaries

| Source | Revision / files | What was actually inspected |
| --- | --- | --- |
| [MostWantedRacerAITraffic](https://github.com/gaycoderprincess/MostWantedRacerAITraffic/tree/a6d9e1f03fac2e4644edd5f78bf3fe8ba542cdbc) | `a6d9e1f03fac2e4644edd5f78bf3fe8ba542cdbc`, main.cpp, README, LICENSE | A published traffic-goal patch and speed override; not built or executed here |
| [NFSMW reconstruction](https://github.com/dbalatoni13/nfsmw/tree/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c) | `src/Speed/Indep/Src/AI/Activities/AITrafficManager.cpp`, AI/Common/AIVehicleTraffic.cpp, AI/Common/AIVehicleRacecar.cpp | Reconstructed traffic reuse, spawn/retirement and racer physics policy; WIP and multiple platforms |
| Same reconstruction | World/TrackStreamer.cpp/.hpp, World/VisibleSection.hpp, World/Common/WRoadNetwork.cpp, Ecstasy/eStreamingPack.cpp | Section-state predicates and road-nav side effects; no new native address is authorized by these files |
| [Native Free Roam Racer / Limit Breaker](https://github.com/Zakkey250/MW-NativeFreeRoamRacer/tree/c8e728809bdc77bfd11e31d975bb2bc7e0cce2bd) | ConstructionLiveSet.inl; LB docs/Population-Reservation-V1.md, Resource-Limits-V1.md, PVEHICLE_XREF_ANALYSIS.md, ACCEPTED_BASELINE_2026-09-20.md | Author's lifecycle/capacity analysis and optional API contracts; original code not copied; gameplay results are the author's reports |
| [MWSDK](https://github.com/TsyVM/MWSDK/tree/6db158647fe05a3d1cbbfee38f0f5d1e91b2f473) | include/mwsdk/game/mw05.hpp, mw05_db.inl | Existing pinned live-IVehicle count/data accessor used by our observation probe |
| [World-streamer notes](https://github.com/s-b-repo/nfsmw-2005-re/blob/0dbc9393e263d95d182e6c98bff3dab4140aa2ff/notes/project_world_streamer.md) | `0dbc9393e263d95d182e6c98bff3dab4140aa2ff` | Region/bundle/path leads; do not establish road-point residency or a complete creation API |
| [Open Limit Adjuster releases](https://github.com/gaycoderprincess/MostWantedOpenLimitAdjuster/releases) | Public release notes checked 2026-10-07 | Independent evidence of companion physics/pool/traffic limits; source/binaries not built or tested here |

## 1. A racer goal does not imply a Racer-class vehicle

RacerAITraffic changes traffic StartDriving's goal string to AIGoalRacer and
overrides traffic drive-speed behaviour. It continues selecting Traffic-class
cars in its callback. Its source comments report zero speed during races with
that goal. This is distinct from NFRR's Traffic construction followed by a
Racer driver-class change and AI revalidation.

Inference for our design: driver class, AI host, goal, desired speed and route
mode are separate observations. Neither a Racer enum nor a non-null AI pointer
proves autonomous roaming. The example's speed override is not our tuning
policy. No global traffic-goal patch or speed write is added here.

## 2. A streaming completion flag is insufficient

The reconstruction distinguishes section states UNLOADED, ALLOCATED, LOADING,
LOADED and ACTIVATED. `TrackStreamer::AreAllSectionsActivated()` also counts
out-of-memory sections in its completion arithmetic. Therefore a true global
completion result does not prove a particular spawn point's section is active.
`IsLoadingInProgress()` can service resource loading in its body; despite its
name it is not suitable as an assumed side-effect-free observer call.

VisibleSection helpers distinguish drivable scenery from distant LOD scenery.
A visible section number and a WRoad segment index belong to different
domains. `eStreamPackLoader::IsLoaded()` has an empty reconstructed body, so it
cannot establish a working residency query.

Next proof: map the exact candidate and its footprint to the required drivable
section(s), verify those sections' activated/current resources in the same
world generation, and establish collision availability separately. Before
implementing a reader, verify the PC executable's structures and access path.
No global-idle, visual-LOD or successful ground query substitutes for this.

## 3. Road-nav queries can change state

In the reconstruction `WRoadNav::CanTrafficSpawn()` chooses a random traffic
lane, calls ChangeLanes and stores the lane index. It is not a read-only check.
`InitFromOtherNav()` copies selected scalar road fields, then rebuilds/evaluates
splines and resets the cookie trail. It is not a raw structure copy.

The traffic manager initializes navigation near a traffic center, checks
validity/spawn permission, then checks ground and nearby vehicles. Those are
useful algorithmic leads, not approval to call them on the player's live nav.

Next proof: a separately owned navigation object with a verified constructor,
destructor and callable PC ABI, initialized at a candidate within our existing
distance policy. Only the same object's exact position/segment/lane can provide
`exactRoadGeometry`. SeekAhead/FarFuture coordinates cannot inherit CurrentRoad
width or lane. Do not change the selected Rico/GTI model to simplify this work.

## 4. Inactive traffic is often cached, not destroyed

The reconstructed traffic manager searches for a matching inactive/loading
model before creating a new object. Spawn activation includes road reset,
SetSpawned, Activate and StartDriving. Its retirement path can UnSpawn without
removing the object, while FlushAllTraffic's release path requests Kill.

NFRR's inspected source treats native retirement as deferred, and avoids
constructing while any protected live object has retirement pending. Its
author reports that several companion containers must be expanded together,
not just one visible count. These are source/author findings, not reproduced
disassembly or gameplay in our installation.

Consequences: active count is not registry count, UnSpawn is not removal proof,
and a cleanup request is not immediate free capacity. Observe fresh membership
after engine processing before reuse/construction. Retain our explicit cleanup
and repeated-registry-removal contract. Do not reclaim vanilla racers or traffic
to make room for the first prototype.

## 5. Population, physics and audio are different resources

LB's public capacity reports separate logical admission, backing storage and
audio capabilities. Its reservation docs describe an expiring single-owner
request and warn that reserving an entire rival deficit can starve traffic.
Its accepted-baseline document reports stalls with CarLoader expansion enabled
and rejects that expansion for distribution. Open Limit Adjuster's notes also
describe separate traffic and physics-object fixes.

Next proof: count every live registry member, including inactive/loading cars;
determine native admission and each required resource independently. An optional
provider must be validated by ABI/capability/size and current status, not a
filename or release number. No provider integration or limit patch is added.
Start with one owned rival and preserve native traffic/police capacity.

The reconstructed racer Update can switch to simple physics when off-world.
That does not prove that stock cars persist or simulate throughout Rockport.
Our eventual distant population needs stable mod-owned identities and a defined
simulation policy; a runtime pointer cannot serve as persistent rival identity.

## Applied now: reject changed live-list snapshots

The post-race observer now captures count/storage and every slot, including
inactive slots, then rereads membership and checks the final header. Incomplete,
null, duplicate, relocated, resized or reordered/replaced lists revoke the
sample. Logs expose registryStatus and world/NIS/fade/loading revocation.

Portable tests cover same-count replacement, reordering, storage changes,
partial reads, null/duplicate slots, oversize and complete empty lists. Two
equal reads are still not an atomic snapshot: an unobserved remove/reinsert
can occur between reads. No result grants ownership or uninterrupted lifetime;
`lifetimeProven=0` remains explicit.

## Next experiment order

1. Finish exact-executable ABI/layout work for owned navigation and point-to-
   drivable-section association; keep new addresses observational until verified.
2. Collect post-race transitions and distinguish rejected samples from no
   surviving numeric identities when the owner returns to the PC.
3. Establish an unchanged-world candidate with exact road, streaming, complete
   displayed-view invisibility, collision ground/grade and selected-model OBB.
4. Verify one owned vehicle's constructor, fresh registries, AI activation and
   deferred cleanup before a create/drive/remove target-machine experiment.
5. Only after that lifecycle succeeds, connect one rival to encounters and
   measure traffic/police/audio capacity under actual pursuits.

No new game execution, target log or native creation is claimed by this report.
