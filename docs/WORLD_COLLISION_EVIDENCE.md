# World Collision Evidence

v0.0.21 adds a deliberately narrow read-only bridge to MW05 world collision.

## Address provenance

Free Roam Rivals supports exactly:

```text
Need for Speed: Most Wanted 1.3
RELOADED Proper No-CD
MD5 C0516B485065FABDD69579816B5DF763
```

A public MW05 project that explicitly requires that same executable uses:

```text
WCollisionMgr::CheckHitWorld = 0x007854B0
```

The independent MW05 reconstruction also provides the method semantics and
call-site behavior.

The executable guard still runs before runtime hooks are installed.

## Why not use the SDK WorldCollisionInfo directly?

Public SDK declarations disagree on the full result layout.

The reconstructed game structure is 0x58 bytes and places:

```text
collision point  @ 0x00
normal           @ 0x10
barrier data     @ 0x20 .. 0x47
world object     @ 0x48
distance         @ 0x4C
animated         @ 0x50
type             @ 0x51
collision inst   @ 0x54
```

v0.0.21 uses an internal raw 0x58-byte structure with compile-time offset
assertions so the engine cannot write past an abbreviated result object.

## Thread safety boundary

The normal runtime sampler runs from D3D9 EndScene. WCollisionMgr is **not**
called there.

Instead:

```text
render sampler
  -> queue POD request
  -> SRWLOCK mailbox
  -> confirmed input/gameplay callback
  -> CheckHitWorld
  -> SRWLOCK result
  -> render sampler logs result
```

The gameplay callback consumes requests only after FrameTick and input-poll
thread IDs are both observed and equal.

The feature is also explicitly opt-in:

```ini
[Diagnostics]
FrameTickProbeEnabled=1
WorldCollisionDiagnosticsEnabled=1
```

Both are disabled by default in normal builds.

## Ground evidence

The MW05 reconstruction of `GetWorldHeightAtPointRigorous` falls back to a
vertical segment:

```text
start = candidate.y - 2
end   = candidate.y + 1000
primitive mask = 1
```

v0.0.21 reproduces only that read-only CheckHitWorld query.

A valid ground result requires:

- the call address to be executable;
- the call to complete without SEH failure;
- a hit;
- `hitType == 1` (world face);
- finite collision point and normal;
- a non-degenerate normal.

The normalized normal also yields a unitless grade:

```text
grade = sqrt(nx^2 + nz^2) / abs(ny)
```

No metre conversion is needed for grade.

## World occlusion

A second query can test the segment from the player's physical vehicle position
toward the candidate. When a learned rival footprint exists, the target is
raised by its half-height so the line is not deliberately aimed into the road
surface.

Primitive mask 3 checks world faces and barriers, matching reconstructed MW05
AI line-of-sight call sites.

This evidence means only:

```text
world line blocked / world line clear
```

It does **not** mean:

```text
camera-visible / camera-hidden
```

The camera may be elsewhere and frustum projection is a separate problem.
Therefore v0.0.21 never sets `visibilityVerified` from this query.

## Remaining spawn evidence

After v0.0.21, the important unresolved live gates are:

- physical world-unit-to-metre calibration;
- real camera/frustum visibility;
- streaming/spooling state;
- binding the actually selected rival model to the already implemented
  pre-construction footprint overlap path.

No unresolved gate is inferred optimistically.


## v0.0.22 mailbox lease

Every POD request/result carries a RuntimeEvidenceStamp. Requests older than
500 ms, or with changed generation/player/road/race/profile identity, are
discarded before CheckHitWorld. A fresh GameBridge sample on the confirmed
gameplay thread verifies safety again; a render observation alone is not
sufficient. New render samples replace pending requests. Generation changes
and unsafe samples invalidate queued/results state. Historical ground evidence
cannot carry readiness into a later world. This is still a diagnostic lease,
not a final candidate-bound spawn authorization.


## v0.0.28 target correction: UMath storage order

The all-diagnostics v0.0.27 target capture exposed an ABI mismatch in the raw
bridge. NFSPluginSDK's shared MW05 `UVector3/UVector4` types provide logical
`x/y/z` fields but declare their physical storage as `y,z,x,(w)`. The
previous raw bridge used `x,y,z,w`, so the engine received permuted segment
bytes even though the logical C++ values were correct.

v0.0.28 mirrors the physical `y,z,x,w` order explicitly and converts back to
logical x/y/z when publishing collision samples. The vertical rigorous-ground
probe still changes logical Y exactly as the reconstruction does; only ABI byte
layout is corrected.

The v0.0.27 capture also proved the call address and gameplay-thread ownership:
queries completed without SEH failure and world-line occlusion could be
interpreted, while every ground sample remained unverified. The next target run
must confirm `ground=valid` before this evidence is promoted any further.


## v0.0.29 coordinate correction

The v0.0.28 target run confirmed that the live collision call can return a
world-face hit, but the logged ground result had an implausible
`groundDeltaWorld=6.288` and `grade=20.567`.

A second source audit resolved the contradiction:

- reconstructed MW05 PC `UMath::Vector3/Vector4` uses `x,y,z,(w)`, with Y
  as the vertical axis;
- the pinned NFSPluginSDK wrapper declares vector members in `y,z,x` order.

Engine calls writing X/Y/Z bytes into an NFSPluginSDK vector therefore appear
through those field names as `value.y/value.z/value.x`. v0.0.29 canonicalizes
that wrapper representation immediately at every NFSPlugin probe boundary.
Domain geometry is now always MW simulation X/Y-up/Z.

The v0.0.28 raw collision shim happened to emit correct engine bytes because
both the input domain vector and the raw mirror were permuted. v0.0.29 removes
that double permutation: canonical domain X/Y/Z enters a conventional raw
x/y/z/w engine vector. This preserves the successful byte-level call while
making vertical ground queries, normals, OBB axes/extents and camera mapping
semantically correct.
