# Selected rival and evidence lifetime

v0.0.22 connects the existing procedural generator to one pending first-spawn
plan. The plan retains GeneratedRival.rivalId and GeneratedRival.vehicleKey
per pseudonymous career profile. The initial seed is derived from that profile.
Partial career progress/district are not yet read, so the experiment uses Tier
1 before verified career completion; after completion it can use Tier 1-5.
This is a pending experiment plan, not persisted rival ownership or a live NPC.

The selected vehicle is resolved through VehicleCatalogProbe. Only that key
is queried in VehicleFootprintLearner. A different verified catalog model can
never satisfy this rival's footprint requirement. The model stays selected
while measurements accumulate; missing models/footprints do not trigger rerolls.
A future persistent garage must provide its active vehicle to the same contract.

SelectedRivalEvidence builds the road-aligned OBB using the **maximum** accepted
half-extents, then runs full fleet SAT. Exact WRoadNav association, a nonzero
rival/model identity, a consistent selected-model footprint and a complete fleet
are required. Later inconsistent measurements revoke verification.

The returned RoadCandidateEvidence promotes only overlapVerified and
 overlapsLiveVehicle. Ground, streaming and visibility remain independent;
metric calibration and final candidate remain fail-closed. Runtime evaluates
this diagnostic through the existing spawn-promotion contract.

World collision requests/results now expire after 500 ms and bind to world
generation, both player identities, road network, race status and profile.
The gameplay consumer samples GameBridge again before calling CheckHitWorld.
Unsafe transitions discard the mailbox; footprint/motion learning resets on a
new generation. Fresh results are diagnostics for their recorded coordinates;
they must not be combined with a different candidate later.

## Research carried forward

The pinned MWSDK camera header provides read-only player camera and view matrix
access. Its render coordinates are Z-up; vehicle simulation coordinates are
Y-up. A camera gate must establish the conversion and classify the whole
selected footprint, including alternate/mirror views, before granting off-screen
spawn evidence. World collision occlusion remains separate.

The public MW05 reconstruction exposes TrackStreamer and section Status values
UNLOADED/ALLOCATED/LOADING/LOADED/ACTIVATED. Scenery visibility alone does not
prove activation. Exact PC address/layout and position-to-section association
remain required before streamingVerified can be enabled.
