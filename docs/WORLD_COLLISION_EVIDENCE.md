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
