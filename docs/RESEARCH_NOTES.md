# Research Notes

Last reviewed: 2026-10-06.

## Verified public foundations

### s-b-repo/nfsmw-2005-sdk

A native C/C++ SDK targeting NFSMW `speed.exe` PE32/i386. It provides typed globals/functions, hook helpers, AOB scanning, D3D9 hooks, input helpers, event helpers and generated address tables derived from its reverse-engineering companion project.

Useful for this project:

- ASI entry scaffold
- MinHook-backed hooks
- D3D9 overlay
- input polling
- event subscription
- struct offsets
- fail-closed signature checks

Source:
https://github.com/s-b-repo/nfsmw-2005-sdk

### s-b-repo/nfsmw-2005-re

Reverse-engineering documentation maps engine systems including:

- AI goals/actions
- world streamer and AI path graph
- vehicle physics
- render/D3D9
- input
- event buses
- UI

Source:
https://github.com/s-b-repo/nfsmw-2005-re

### TsyVM/MWSDK + MWEncyclopedia

MWSDK exposes verified runtime/file tooling and a reference layer containing vehicle/front-end database layouts. Its documentation also covers road-network/world-data tooling.

Sources:
https://github.com/TsyVM/MWSDK
https://github.com/TsyVM/MWEncyclopedia

## Facts we should not assume yet

The following must be demonstrated in our target executable before production code depends on them:

- exact safe function for deleting a specific career-owned car
- exact safe function for copying every customization field into a newly owned car
- stable arbitrary AI racer spawning sequence in free roam
- reliable engine-native control of an AI car to an exact parking pose
- stock save/autosave synchronization after custom garage transfers

## Research order

1. establish per-frame hook
2. establish game/free-roam state
3. player vehicle pointer
4. live vehicle enumeration
5. racer AI control / spawn
6. input
7. D3D9 HUD
8. road graph
9. camera control
10. economy
11. garage ownership
12. save transaction verification

This order deliberately postpones destructive state.
