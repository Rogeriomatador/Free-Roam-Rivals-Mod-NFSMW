# Vehicle Spatial Evidence

v0.0.19 adds a read-only spatial layer for live PVehicle objects.

The goal is to answer one prerequisite of safe rival spawning:

> Is the candidate space already occupied by another live vehicle?

## Runtime source

The probe uses the same NFSPluginSDK PVehicle registry that already provides
the independent player cross-check.

For each enabled, active, non-destroyed vehicle it reads only:

- `IRigidBody::GetPosition()`
- `GetRightVector()`
- `GetUpVector()`
- `GetForwardVector()`
- `GetDimension()`

No rigid-body setter or collision mutation is called.

If a registry entry is active but its spatial state cannot be read safely, the
fleet result becomes incomplete and overlap evidence fails closed.

## Dimension semantics

Public MW05 reconstruction work gives strong evidence that rigid-body
`GetDimension()` represents local half-extents.

Examples from reconstructed game code use:

```text
position - up * dimension.y
position - forward * dimension.z * 0.75
```

for bottom/scrape contact placement, and collision boxes consume the same
dimension vector.

The reconstructed collision asset format also names its stored values
`fHalfDimensions`.

Free Roam Rivals therefore maps:

```text
dimension.x -> local right half-extent
dimension.y -> local up half-extent
dimension.z -> local forward half-extent
```

Target runtime logs remain the final sanity check for the supported executable.

## Oriented boxes

Each readable live vehicle becomes:

```text
center
right axis
up axis
forward axis
half extents
```

The domain layer rejects non-finite values, non-positive extents and corrupted
non-orthogonal bases.

## Queries

Two pure-domain query types are implemented.

### Point occupancy

Given a world point, the evaluator reports:

- whether the registry evidence is complete;
- whether the point is inside any live vehicle OBB;
- the containing vehicle identity;
- the nearest point-to-vehicle-box separation in world units;
- unreadable/invalid vehicle count.

This is already wired to road-candidate heartbeat diagnostics.

### Candidate footprint overlap

A full 15-axis separating-axis-theorem OBB test is implemented.

When the candidate rival's own oriented footprint is known, the evaluator can
prove whether it overlaps any live vehicle and can identify the blocking
vehicle.

This path is intentionally not promoted to `RoadCandidateEvidence.overlapVerified`
yet because the selected rival's collision footprint is not currently available
before PVehicle construction.

## Why point-clear is not enough

A spawn center can be outside every existing car while the newly created car's
body would still intersect one of them.

Therefore:

```text
point clear != footprint clear
```

The mod logs point occupancy for diagnosis but keeps the real overlap gate
false until both boxes are known.

## Readiness

v0.0.19 adds:

```text
VehicleSpatialEvidenceUnavailable
```

to the construction-readiness chain.

The runtime marks live spatial evidence observed only after:

- the PVehicle registry terminates normally;
- every relevant active vehicle has a valid OBB read.

## Next step

The best remaining path is to obtain the chosen rival's collision bounds
without constructing it.

Public reconstruction shows vehicle code looking up collision geometry by
vehicle model and exposes collision bounds with `GetHalfDimensions()`, but
Free Roam Rivals will not call that path until the exact lookup ABI/address is
verified for the supported executable.

Until then, candidate-footprint overlap remains fail-closed.


## v0.0.20 — learned pre-construction footprints

The missing candidate footprint can now be learned without calling an unverified
collision-geometry lookup.

Every valid live OBB also records `IVehicle::GetVehicleKey()`. Public
reconstruction shows this key is the pvehicle collection key used by traffic
patterns and the attribute database.

The learner groups rigid-body half-extents by that stable key and requires four
samples whose maximum component spread is within 3%.

Configured catalog names such as `supra` and `rx7` are resolved through the
live pvehicle database to the same key.

Once a catalog model is verified, the runtime can construct a temporary
road-aligned OBB from:

```text
candidate road position
candidate road forward
learned model half-extents
```

and run the existing SAT test against the complete live fleet.

This is still read-only. It does not instantiate a PVehicle. The final spawn
candidate remains blocked until the other independent gates—metric scale,
streaming, ground and visibility—are also proven.
