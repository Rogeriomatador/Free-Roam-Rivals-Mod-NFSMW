# Primary camera frustum evidence

v0.0.23 adds an opt-in guarded read-only camera bridge:

```ini
[Diagnostics]
CameraFrustumDiagnosticsEnabled=1
```

No camera hook, camera ownership change or transform write is installed. The
sampler reads with the road/fleet observation from EndScene. This is deliberately
separate from gameplay-thread-only WCollisionMgr queries.

## Exact PC source

The already pinned MWSDK commit
`6db158647fe05a3d1cbbfee38f0f5d1e91b2f473` supplies guarded reads in
[mw05_camera.hpp](https://github.com/TsyVM/MWSDK/blob/6db158647fe05a3d1cbbfee38f0f5d1e91b2f473/include/mwsdk/game/mw05_camera.hpp)
and data provenance in
[mw05_db.inl](https://github.com/TsyVM/MWSDK/blob/6db158647fe05a3d1cbbfee38f0f5d1e91b2f473/include/mwsdk/game/mw05_db.inl).

| Data | Verified SDK address/offset |
|---|---|
| eViews | 0x9195E0, stride 0x70 |
| Player view | eViews[1] = 0x919650 |
| Player camera pointer | player view + 0x40 = 0x919690 |
| View ID / active | +0x04 / +0x08 |
| PlatInfo pointer | +0x00 |
| Camera render-space eye | Camera +0x40 |
| View / projection / view-projection | PlatInfo +0x00 / +0x40 / +0x80 |

The executable guard still gates all runtime installation. The bridge uses the
SDK's guarded copies and re-reads camera/PlatInfo identities after copying.
Camera data in MWSDK is explicitly a research tier, so target-installation
validation remains required even when the mathematical checks pass.

## Coordinate proof

The MW05 reconstruction at `dbalatoni13/nfsmw`, commit
`1f2cdd7996791c81a580b3f7b36b44d4f9f6719c`, identifies
[eSwizzleWorldVector](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/src/Speed/Indep/Src/Ecstasy/Ecstasy.hpp)
as a call to
[bConvertFromBond](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/src/Speed/Indep/bWare/Inc/bMath.hpp).
Its vector mapping is:

```text
render = (simulation.z, -simulation.x, simulation.y)
```

[CarRenderConn.cpp](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/src/Speed/Indep/Src/World/CarRenderConn.cpp)
uses the inverse before world-ground queries and the forward conversion after
reading the simulation ground height. This supports the coordinate semantics;
no platform function addresses are imported from the reconstruction.

## Fail-closed mathematical checks

- primary view ID=1, active, available and mapping established;
- finite eye and all matrix elements;
- rigid affine view, orthonormal basis;
- perspective D3D projection shape;
- eye transforms to the origin of view space;
- copied ViewProjection matches View * Projection;
- all six extracted clip planes are nondegenerate;
- valid selected-model OBB basis and dimensions.

The classifier transforms center and axes using the same mapping. It extracts
inward planes for row-vector D3D clip inequalities (-w<=x/y<=w, 0<=z<=w).
A box is outside only if its whole support interval is strictly separated from
one plane. Contact and partial intersection remain potentially visible. A
center-only projection cannot pass this test.

Possible logs:

- `unverified`
- `outside_primary_frustum`
- `potentially_visible`

## What remains unverified

`spawnVisibilityVerified` remains false. The result covers the selected model's
**collision footprint** in the **primary view**, not the complete rendered car
in every displayed view. We must still prove:

1. target-machine camera/matrix coherence with current mods and cameras;
2. exact PC rear-view/mirror activation and all displayed-view coverage;
3. conservative visual bounds including bodykits, spoilers and render offsets;
4. candidate-bound freshness at the eventual gameplay construction callback.

The public reconstruction has a separate EVIEW_PLAYER1_RVM view and a car
rendering path that adjusts the body matrix by model offset. Those are evidence
that ignoring mirrors or assuming collision dimensions equal rendered bounds
would be unsafe. A primary-frustum result therefore does not set
RoadCandidateEvidence.visibilityVerified yet. World occlusion also cannot do so.

## Streaming research carried forward

[TrackStreamer.hpp](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/src/Speed/Indep/Src/World/TrackStreamer.hpp)
has separate section statuses: UNLOADED, ALLOCATED, LOADING, LOADED, ACTIVATED.
It also has SectionNumber, pMemory, LoadedSize and pending section counts.
[VisibleSection.hpp](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/src/Speed/Indep/Src/World/VisibleSection.hpp)
provides drivable boundaries/position-to-section concepts.

The PC RE notes
[project_world_streamer.md](https://github.com/s-b-repo/nfsmw-2005-re/blob/main/notes/project_world_streamer.md)
name GetTrackStreamerSingleton at 0x79CA30 but do not establish the complete ABI,
section-array layout or exact candidate-to-section lookup for our executable.
Global asset pending counters in
[project_streamer_anchors.md](https://github.com/s-b-repo/nfsmw-2005-re/blob/main/notes/project_streamer_anchors.md)
are not per-position activation evidence.

No guessed TrackStreamer layout or new raw engine call is added. Distance,
road-nav validity, primary-frustum state, a world collision hit or zero global
pending loads cannot establish streamingVerified. That gate stays false until
exact PC layout/address and position-to-activated-section association are proven.
