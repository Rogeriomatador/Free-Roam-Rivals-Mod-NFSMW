# Actual PC capture: v0.0.30-dev, 2026-10-08

Input: FreeRoamRivals(9).log, 2247 lines, SHA-256
`9dfdefefe4b7ad0dcaf8cc6f73496a8cece61cb243145d669700f9c0f67cc5e8`.
User reports following native opponents for a long time. Raw logs are not published.
Target MD5 C0516B485065FABDD69579816B5DF763, 6029312 bytes accepted.
Main-loop callback delivered through recognized WidescreenFix chain.

736 complete Stable PostRace observations: 192 Racing, 544 Roaming, zero matches.
Last Racing 03:28:46.341; fade revoked at 03:28:46.839; first Roaming
03:28:48.845; last Roaming 03:33:20.710 (about 4m32s).
The v30 fade guard called reset(), erasing the race cohort before Roaming.
Longer observation cannot restore that cohort. This is a diagnostic failure,
not evidence that native opponents disappeared.

Runtime state changes report racer count 3 at 03:28:48.873, 2 at 03:30:08.281,
1 at 03:30:17.234, 0 at 03:33:18.836. Counts do not identify individual cars
or distinguish destruction from inactivity/classification changes.
Motion estimator internally promoted metricVerified at 03:29:18.649 with
worldUnitsPerMeter 0.999888957; independent physical units remain a separate question.
All 47 collision result summaries stayed pending_or_unavailable.

## v31 change and limits

A fade-only guard can retain plain Racing values for ten seconds from its first
fade sample. It reads no vehicle payload during fade. NIS, loading, invalid world,
player cross-check failure, incomplete registry, changed context or long sampling
gap still revoke. Repeated fade samples cannot extend the total deadline.
A resume must be Roaming with the same context and freshly guarded live registry.
Matching numeric values across the interruption does not exclude address reuse
(ABA); correlationInterruptedByFade=1 and lifetimeProven=0 remain explicit.
No historical pointer is dereferenced, no native opponent becomes mod-owned.
First resumed sample does not measure motion from the pre-fade race position.

SpawnSafety now refuses Unknown, Active, Cooldown and Busted pursuit states.
The exact-executable native pursuit reader is not yet verified: runtime stays
Unknown. Heat and cop counts must not substitute for verified pursuit state.
This policy does not implement racer police behavior, cleanup ownership, roadblock
avoidance or arrest/escape lifecycle. Construction and AI mutation remain disabled.

Next PC test after a published v31 package: ordinary career race, follow an opponent
for 30–60 seconds, send full log. Expect fade retention/capturedRaceCount and
interrupted match records if exact identities/context persist. Zero matches still
require checking actual cohort count and context. Pursuit behavior must be tested
separately after the reader and ownership lifecycle are implemented.
