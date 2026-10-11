# v0.0.45-dev — verifiable rival interaction and HUD feedback

## Goal
Rico continues to drive using the existing *native Racer AI*. v45 changes the mod-owned encounter's **responses to observed measurements and events**, not the game's steering, acceleration, or driving goal. No virtual horn, brake, ram, player pursuit or road path is fabricated.

## HUD changes
- Approach: Rico cruising, over 200m distant, on a different road elevation, getting closer, moving away, or genuinely being followed. Distances and trends come only from current verified telemetry. Distinct state labels are smoothed for 0.5 seconds.
- Ready: only when the domain says the existing outrun G challenge is physically available. The verified G challenge indicator is green.
- Rejections: exact live feedback about out-of-range, vertical difference, wrong direction, speed mismatch, cooldown, pursuit, or missing verified physics. A new unsafe challenge is never started.
- Overtakes: show player/rival lead change for 2.5 seconds only if the domain confirms a close same-trajectory overtake. No inferred overtakes from camera position.
- Finish/cooldown: scoring, history, native Racer AI and native vehicle ownership remain as implemented in v44.

Approach estimation rejects implausible discontinuities and zero-delta snapshots. It resets on invalid world/player/rival evidence and on profile changes; no stale marker should be shown.

## Not implemented: native motor decisions
A real AI reaction such as chasing the player, choosing alternative turns, accelerating on challenge or blocking the road would require verified MW05 native AI control structures and lifecycle-safe game-thread writes. The owner captures verify the existing Racer goal, not a safe dynamic-player-target/steering API. Consequently v45 never changes the native goal/vehicle controls. It also does not resolve road_reset_rejected, native streaming lifetime or the one-owned-vehicle limit.

## Install without editing the configuration
- EXISTING mod: use FreeRoamRivals-v0.0.45-dev-upgrade.zip. It contains only the new ASI, preconfigured main FreeRoamRivals.ini and documentation. It leaves Rivals.ini, VehiclePools.ini, UndergroundBlacklist.ini and saves untouched.
- CLEAN install: use FreeRoamRivals-v0.0.45-dev.zip with all auxiliary INIs.
- Close the game, back up any customized main mod INI, and extract the selected ZIP into the directory containing speed.exe. The play INI includes F8, G and Ctrl+Shift+R; no renaming or manual INI changes.

## Tests
Windows CI compiles the Win32 ASI and runs CTest/Python suites, including new reaction telemetry and existing D3D HUD state-restoration checks, plus ZIP validation. This does not prove native driving behavior in the owner's game.
Target test: drive a GTI to calibrate, F8 to request Rico. Approach, follow, check the overpass vertical status and see G turn green only when genuinely challenge-ready. Start an outrun, record actual close overtakes/leader change, result and rematch cooldown. Verify game/mod profiles and saves remain intact. After native faults, do not try to force spawning; relaunch for a separate test.
