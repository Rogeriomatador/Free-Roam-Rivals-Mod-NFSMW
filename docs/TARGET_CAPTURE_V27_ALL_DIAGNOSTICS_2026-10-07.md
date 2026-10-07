# Target capture — 2026-10-07, v0.0.27 all diagnostics

Source: user-supplied `FreeRoamRivals(6).log`. The original file is not
republished here.

## Confirmed healthy

- All diagnostics were enabled together: render, fallback input, FrameTick,
  motion capture, primary camera and world collision.
- The recognized Widescreen Fix main-loop chain installed with MH_OK.
- Gameplay-loop/frame/render observations remained on one consistent thread for
  more than five minutes.
- 24 fallback-key rising edges were observed.
- Motion capture remained operational across four Free Roam generations and
  correctly revoked safe sampling during fade/transition states.
- Normal driving produced a stable displacement/speed consistency ratio near
  1.0. Physical metre calibration is still deliberately unverified.

## World collision finding

The address and gameplay-thread mailbox worked, but every returned ground
sample remained unverified while world-line queries completed. Source audit
against the pinned NFSPluginSDK then identified the raw ABI error:
`UVector4` is physically stored as y,z,x,w, not x,y,z,w.

v0.0.28 corrects that byte order without changing query ownership or relaxing
any evidence gate.

## Spatial finding

The old PVehicle instance pool grew to 74 entries while MWSDK's verified live
vehicle list reported only 20 live vehicles. The spatial probe therefore
classified many stale pool entries as active and accumulated 53 failed reads in
one heartbeat. v0.0.28 switches occupancy enumeration to the verified live
IVehicle list and uses virtual read access only.

## Camera finding

Primary-camera matrix coherence succeeded intermittently on the target, proving
the guarded camera path can produce coherent samples. Candidate visibility
remained unverified because the exact selected GTI footprint was never verified,
and complete displayed-view/render-envelope coverage is intentionally still
missing.

No vehicle construction or AI mutation occurred in this capture.
