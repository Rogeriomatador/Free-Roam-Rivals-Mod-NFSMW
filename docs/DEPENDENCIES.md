# Dependencies and Evidence Policy

Free Roam Rivals uses multiple public projects because each is strongest in a different area. Release builds pin exact commits instead of following moving branches.

## nfsmw-2005-sdk

Repository: https://github.com/s-b-repo/nfsmw-2005-sdk

Pinned commit: 3b3d05b9194844883aa42d75dd7aae66838092ff

Role:
- ASI entry point
- MinHook wrapper
- AOB/signature infrastructure
- input poll hook
- D3D9 EndScene hook
- verified global/function constants

Policy: use as the plugin and hook foundation.

## MWSDK

Repository: https://github.com/TsyVM/MWSDK

Pinned commit: 6db158647fe05a3d1cbbfee38f0f5d1e91b2f473

License: MIT.

Role:
- provenance-first retail-v1.3 addresses
- verified live IVehicle registry
- driver-class identification
- player vehicle resolution
- safe/read helpers
- road-network singleton
- camera research layer
- offline traffic paths and CARP graph

Policy: prefer MWSDK when it contradicts an older community assumption about a live address.

Concrete example: MWSDK's verified live IVehicle registry supersedes the older PVehicle instance-table approach for player discovery.

## NFSPluginSDK

Repository: https://github.com/berkayylmao/NFSPluginSDK

Pinned commit: d238bdffc648498840d17133edb256fa0310d355

License: BSD 3-Clause.

Role:
- mature typed NFSMW structures
- PVehicle / AIVehicle API vocabulary
- GRaceStatus
- career/front-end/garage structures
- established gameplay calls

Policy: use typed structures and established calls behind our validation layer. Destructive operations need corroboration before shipping.

## NFS Chat Chaos Mod

Repository: https://github.com/berkayylmao/NFS-Chat-Chaos-Mod

Role: behavioral research only.

It demonstrates practical runtime behavior such as constructing free-world vehicles, grounding them, using created AI/cops, and cleaning up vehicle lifecycle.

License: AGPL.

Policy: do not copy its implementation code into Free Roam Rivals. Use it only to confirm that an engine path has been exercised by a real mod, then implement our own code from SDK/API knowledge.

## nfsmw-2005-re

Repository: https://github.com/s-b-repo/nfsmw-2005-re

Role: deep reverse-engineering documentation covering GameFrameTick, AI goals/actions, race status, event buses, world streamer, physics, camera/render, save format and career milestones.

Policy: use it to understand lifecycle and select safe hook points before calling raw functions.

## Evidence order

When sources disagree, prefer:

1. verified retail bytes / exact target executable
2. independently reproduced runtime behavior
3. MWSDK verified database
4. nfsmw-2005-re reverse-engineering documentation
5. mature NFSPluginSDK typed structures
6. shipping/open-source mod behavior
7. community anecdote

No destructive feature ships based only on anecdote.
