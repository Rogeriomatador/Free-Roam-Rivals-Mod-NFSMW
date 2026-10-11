# NFSMW: Free Roam Rivals

**Free Roam Rivals** is a native mod for **Need for Speed: Most Wanted (2005) PC v1.3** focused on making Rockport feel alive outside scripted races.

The goal is not to place static race markers on the map. Rivals should exist as persistent street racers: they drive around Rockport, notice the player, provoke or ignore them according to personality, challenge them naturally, race without loading screens, remember wins and losses, and eventually support high-stakes wagers including pink-slip car races.

## Vision

A normal free-roam session should be able to produce a story like this:

1. A tuned Supra passes the player in Camden.
2. The driver slows down and waits.
3. The player approaches and honks.
4. The rival responds and leads the player to a nearby safe staging point.
5. Both cars stop side-by-side.
6. A real-time cinematic introduces the rival and wager.
7. The player chooses cash, reputation, or — when eligible — a pink-slip race.
8. The player confirms, both engines rev and the countdown runs.
9. The race starts without loading or leaving free roam.
10. Police may become involved and the race continues.
11. The result persists. A rival who wins the player's car may later be seen driving it.

The project is deliberately designed as a **world system**, not a collection of fixed races.

## First supported game target

The first target is NFSMW PC v1.3. Development is being tested against the executable currently used by the project owner:

```text
MD5: C0516B485065FABDD69579816B5DF763
```

Support for additional 1.3 executables will be added only after their addresses/signatures are verified.

## Planned pillars

- Living rivals in free roam
- Natural challenge flow via proximity + horn
- Rival personalities and memory
- Outrun and destination races without loading screens
- Cash, reputation and pink-slip stakes
- Last-car protection: the player can never wager their only car
- Persistent rival garages and economy
- Rivals can potentially drive cars won from the player
- Real-time staging cinematics
- Police integration
- Street meets and rival-vs-rival events
- A real post-career secondary ranked Blacklist found through Free Roam
- Safety-first save handling and rollback

## Current status

**v0.0.52-dev:** novo indicador **verde RICO** (bússola separada ao lado do minimapa nativo, com direção relativa à orientação física do jogador), **seta amarela de trilha observada** quando o jogador segue Rico em Outrun e aviso de **imobilização observada** após 12 s quase sem movimento. A posição de Rico é obtida de leitura verificada, mas o marcador ainda NÃO integra o minimapa FNG original. Nenhuma alteração de volante/objetivo da IA foi realizada. [Guia completo](docs/RICO_HUD_AND_STUCK_V52.md).


**v0.0.51-dev:** F8 no modo PLAY próximo agora aceita **apenas pistas à frente da orientação física do carro do jogador**, entre aproximadamente 20 e 115 m à frente e 10–30 m de corredor lateral. O modelo continua sendo GTI; colisão, terreno e criação nativa protegida permanecem iguais. Se não encontrar pista à frente, informa e não nasce atrás. Ordena as candidatas dianteiras pela possível visibilidade da câmera e direção/centralidade. [Procedimento de teste](docs/RICO_AHEAD_SPAWN_V51.md).


**v0.0.50-dev:** os novos testes reais confirmaram um GTI ativado a 42,8 m e que acelerou até ~216 km/h antes de perder streaming, mas não provaram que apareceu na imagem; o segundo teste teve `constructor_null` sem carro criado. No modo de prova de criação próxima, F8 agora **prioriza ruas potencialmente dentro da câmera** quando esse dado é válido; quando o construtor retorna NULL, registra contagens reais de veículos/capacidade antes da chamada, sem inferir que o pool está cheio nem forçar re-spawn. [Leia os limites](docs/VISIBLE_PROTOTYPE_V50.md).

**v0.0.49-dev:** novo log v48 de 11/10 mostrou que a criação a 118,889 m falhou no `ResetToRoad`: o carro não ficou no ponto esperado, provocando `road_reset_rejected` e bloqueio de F8 por **falha nativa irrecuperável até reiniciar**. A v49 registra deltas e seed exatas para encontrar a causa e **não aceita F8 em fila enquanto a factory estiver bloqueada**. O critério de posição e de remoção continua estrito. [Diagnóstico e teste](docs/ROAD_RESET_EVIDENCE_V49.md).

**v0.0.48-dev:** a captura real de 11/10 mostrou Rico **criado a 30 m** e afastando-se dirigindo até mais de 1,8 km, onde a física ficou indisponível e a solicitação de remoção tardia ficou bloqueada. O protótipo próximo agora tenta uma retirada antecipada APENAS quando o GTI ainda está carregado, 350–850 m distante, fora de vista e com oito raios de oclusão verificados, sem desafio/perseguição; F8 durante retirada fica em fila e só avança após remoção confirmada nas duas registries nativas. Sem teleportes, exclusões forçadas ou promessas de persistência de streaming. [Relato, controles e limites](docs/GUARDED_REENCOUNTER_V48.md).

**v0.0.47-dev:** F9 também verifica, de modo estritamente **somente leitura**, os arquivos originais CARS/GEOMETRY.BIN, TEXTURES.BIN, GLOBAL e TRACKS que existem na sua instalação, e consulta os nomes internos registrados no runtime. Resultados entram no FreeRoamRivals.log e não habilitam spawns não comprovados: disco + atributo **não garantem residência dos recursos na memória do jogo**. [Pesquisa, limites e instruções](docs/NATIVE_ASSET_INVENTORY_V47.md).

**v0.0.46-dev:** após a primeira calibração física comprovada, o mod salva automaticamente a escala em `scripts/FreeRoamRivals/world-metric-v1.state` e a reutiliza nas próximas inicializações com o mesmo `speed.exe`. Se a prova estiver ausente, corrompida ou incompatível, exige uma calibração normal; não adivinha metros. A confirmação posterior de movimento pode invalidar uma escala contraditória. Nenhuma edição de INI ou cópia manual de arquivos é necessária. **O Golf GTI e o F8 continuam sendo requisitos temporários do protótipo.** [Mais detalhes](docs/PERSISTENT_WORLD_METRIC_V46.md).

**v0.0.45-dev (reaction HUD):** measured approach and separation, following, different elevation, valid G challenge readiness, live rejection reasons and one-shot physically verified overtakes. This is **reactive gameplay UI**, not a new native driving AI: the Racer goal, speed, steering and road navigation writes are unchanged. [v45 reaction evidence/limitations](docs/RIVAL_REACTIONS_V45.md). Ready-to-extract full and existing-installation upgrade ZIPs continue to preserve existing saves/pools. 

**v0.0.44-dev (experimental; ready-to-extract ZIP):** displays active Rico distance and player-travel-relative bearing in the HUD. The play preset binds Ctrl+Shift+R (hold 1.5 seconds) to guarded native removal, and F8 is re-armed only after the mod confirms full retirement and no native fault. Better road-reset failure details are logged. Native faults, streaming, world transitions and normal traffic never trigger automatic reconstruction. See [v44 testing notes](docs/RIVAL_LOCATOR_AND_RETRY_V44.md). The release provides **two ready-to-extract ZIPs**: use `FreeRoamRivals-v0.0.44-dev-upgrade.zip` for an **existing mod installation** (replaces only the ASI and main `FreeRoamRivals.ini`, preserves `Rivals.ini`, `VehiclePools.ini`, `UndergroundBlacklist.ini` and all `saves`), or the full ZIP for a clean install. Back up a customized main INI first; extract into the folder with `speed.exe` and accept replacement. No INI editing, renaming or preset copying is required. F8 creation remains manual and experimental. The full ZIP also offers `FreeRoamRivals-safe.ini` as an inactive alternative.


**v0.0.43-dev — correct native frame time, challenge feedback and F7 conflict removal.**

The v42 owner capture confirms G input and a battle starting, then aborting after 0.259 seconds. Its history file correctly records one interruption. Static inspection found the callback interpreted a native integer fixed-millisecond argument as float seconds. v43 forwards the original stack word unchanged to the game and converts only the FRR callback time using the target's two constants. A regression reproduces the old false discontinuity on normal movement and accepts the same motion with native conversion.

G remains the alternative challenge key; horn integration is unverified. Refusals now show proximity/alignment/speed/cooldown feedback and log explicit reasons; terminal race samples identify safety interruption causes. F7 is no longer polled by default: manual retirement defaults off, including old INIs without the new key. An optional user-selected hold shortcut can be configured; no shortcut is claimed universally conflict-free. Scoring freezes on nonadvancing physical poses with stale nonzero speed.

Use [the play preset](config/FreeRoamRivals-play.ini) as `scripts/FreeRoamRivals/FreeRoamRivals.ini`; F8 creates one stock GTI and G challenges it. [v42 capture and v43 correction](docs/TARGET_CAPTURE_V42_CHALLENGE_TIME.md). The [v42 gameplay procedure](docs/LIVE_RIVAL_V42.md) describes the original integration; its F7 control is superseded in v43. No cash/garage/vanilla save writes. Native roaming AI does not follow a player-selected race route. Tuned rivals, full population/personality/economy and streaming recovery remain pending. This is not a final release; the corrected challenge still requires in-game validation.

## Earlier read-only foundations (v0.0.30 and below)

v0.0.30 adds an optional `Diagnostics.PostRaceRacerDiagnosticsEnabled=1` observation of native racers across stock-race/free-roam transitions. It correlates live vehicle identities, AI interface identities and movement without changing race goals or constructing cars. Loading, incomplete reads and context changes discard correlations; no matches are inconclusive. Target-game validation is pending. See [post-race research and test procedure](docs/POST_RACE_RACER_RESEARCH.md) for pinned public sources, evidence limits and the separate existing Native Free Roam Racers implementation.

The [streaming/roaming/cache investigation](docs/NATIVE_ROAMING_STREAMING_RESEARCH.md) records why global loading completion, Racer class and inactive traffic counts cannot establish a safe creation path. Post-race samples also reject observed live-list storage or membership changes during traversal.

The ASI validates the supported executable, installs observation hooks, separates Free Roam from stock races, enumerates live IVehicle driver classes, independently cross-checks the player PVehicle, observes the road network, reads career cash/car-count/completion, and validates the configured rival-car catalog without mutating the game.

The population layer distinguishes authored persistent rivals, deterministic procedural locals with stable names/personality/vehicle identity, legendary condition-based rivals, and untouched vanilla traffic.

v0.0.10 adds two runtime foundations that were previously only planned:

- live read-only WRoadNav telemetry from the player's native AI object, including current/future segment, lane, road width, curvature and geometry;
- mod-owned per-profile Underground Blacklist persistence in `scripts/FreeRoamRivals/Saves/`, keyed by a one-way pseudonymous hash. The raw NFSMW profile name is never written to the mod save.

This moves staging/route work from abstract road-network availability toward real lane/segment data and gives the post-career ladder a safe persistence path without touching the vanilla save.

v0.0.11 also adds an opt-in, read-only probe at the verified `GameFrameTick @ 0x663D30`. It exists specifically to compare the exact main-loop thread with the existing input and render callbacks before any vehicle creation or AI mutation is permitted. It is disabled by default.

v0.0.14 expands the road-navigation probe with native `SeekAheadPosition`, `FarFuturePosition`, `FarFutureDirection`, segment timing and occlusion evidence. Raw coordinate deltas are explicitly treated as **world units**, not metres. A new `WorldMetricCalibration` gate must be explicitly verified before raw geometry can enter metre-based spawn or staging thresholds.

v0.0.15 adds the first usable challenge input path: a configurable fallback virtual key sampled from the existing game input-poll callback with rising-edge semantics and a bounded atomic queue for the future EncounterDirector. The verified MW05 action table still exposes no native `HORN/HONK` action, so horn integration remains explicitly unproven rather than being faked with another HUD action.

v0.0.16 consolidates the remaining first-spawn prerequisites into a single fail-closed construction-readiness report. It identifies the first blocker among FrameTick enablement/install/observation, main-loop thread confirmation, safe Free Roam, road lookahead, metric calibration and final spawn-candidate promotion.

v0.0.17 adds read-only player-motion cross-checks for `GetSpeed`, `GetSpeedometer`, `GetAbsoluteSpeed`, local/linear velocity and world-position displacement over elapsed time. A statistical observer can establish a stable `worldUnitsPerSpeedUnitSecond` relationship, but it deliberately does not promote that value to metres until the physical speed unit is proven on the target installation.

v0.0.18 adds a typed road-candidate pipeline. `CurrentRoad`, `FutureRoad`, `SeekAhead` and `FarFuture` are represented as different evidence sources instead of interchangeable coordinates. Only a point carrying exact WRoadNav geometry can progress toward spawn/staging, and promotion still requires verified metric conversion plus independent streaming, ground and overlap evidence. Spawn additionally requires verified off-screen visibility; staging deliberately does not, because both cars can drive toward a visible cinematic site.

v0.0.19 adds read-only spatial evidence for the live PVehicle registry. Active vehicles are represented as oriented boxes from their rigid-body position, basis vectors and dimensions. Pure-domain SAT tests can verify OBB-vs-OBB overlap when both footprints are known, while runtime road-candidate diagnostics already report whether a candidate point lies inside any live vehicle and the nearest vehicle-box separation. The final `overlapVerified` spawn gate remains fail-closed until the selected rival's own collision footprint can be obtained before construction.

v0.0.20 closes that specific footprint gap without calling an unverified collision-geometry lookup. Live cars now carry their stable `IVehicle::GetVehicleKey()`. Repeated consistent rigid-body dimension samples are learned per model; configured catalog names are resolved to the same pvehicle keys. Once a catalog model has a verified footprint, the runtime can build a road-aligned pre-construction OBB for that model and test the full box against every live vehicle before construction.

v0.0.21 adds an opt-in read-only world-collision query for the exact supported RELOADED 1.3 executable. The `CheckHitWorld @ 0x7854B0` address is independently used by a public MW05 mod that targets the same MD5. Free Roam Rivals never calls it from D3D9: render sampling only queues a request, and the query is consumed from input/gameplay after FrameTick and input thread IDs match. A vertical world-face hit can now prove ground and derive dimensionless road grade; player-to-candidate world/barrier occlusion is logged separately and is **not** treated as camera visibility.

v0.0.9 also adds a tested, independent **Underground Blacklist** domain that unlocks after the vanilla career is completed. It is not a relabel of the original Blacklist: the mod tracks its own ranks, discovery state, qualification requirements, world-hunt eligibility and future portrait/audio asset keys.

The previous runtime foundations remain in place:

- world-generation tracking so live engine pointers cannot survive a world transition;
- generation-scoped rival runtime handles with destroy-pending invalidation;
- fail-closed spawn environment/candidate gates;
- read-only experimental spawn preflight diagnostics;
- Outrun result/lead/hold/timeout/abort logic;
- staging candidate scoring for straight, wide, streamed, non-junction road sections;
- cinematic staging state flow from search through release;
- mandatory camera/control restoration directives on staging failure or completion.

The next hard runtime milestone remains intentionally narrow:

> Feed a verified road-safe/off-screen candidate into a dedicated gameplay-thread construction probe, prove create → AI → road navigation → cleanup, then allow one native rival to roam.

The render callback remains observation-only. Pink slips and save mutation come later, after the runtime foundation is proven stable.

v0.0.22 retains one deterministic pending rival per profile and uses only its selected vehicle key for footprint learning and overlap promotion. Missing evidence never changes that selection. Collision requests/results carry a 500 ms session lease and are revalidated against the live gameplay state before querying the world. See `docs/SELECTED_RIVAL_EVIDENCE.md`.

v0.0.23 adds opt-in guarded primary-camera/view matrix reads and tests the entire selected collision footprint against the real primary frustum. It validates coordinate mapping, matrix coherence and box-plane separation; primary-camera evidence stays separate from final spawn visibility until mirrors and visual bounds are established. See `docs/CAMERA_FRUSTUM_EVIDENCE.md`.

## Repository map

```text
src/                  native ASI source
config/               default configuration templates
docs/
  VISION.md            design principles
  GAME_DESIGN.md       gameplay systems
  TECHNICAL_FEASIBILITY.md
  ARCHITECTURE.md
  ROADMAP.md
  SAVE_SAFETY.md
  RESEARCH_NOTES.md
  ENGINE_INTEGRATION_MAP.md
  SPAWN_AND_AI_PLAN.md
  RUNTIME_SPAWN_CONTRACT.md
  ROAD_NAV_PROBE.md
  CHALLENGE_INPUT.md
  RUNTIME_READINESS.md
  MOTION_SCALE_CALIBRATION.md
  ROAD_CANDIDATE_PROMOTION.md
  VEHICLE_SPATIAL_EVIDENCE.md
  WORLD_COLLISION_EVIDENCE.md
  PINK_SLIP_ENGINE_RESEARCH.md
  RIVAL_POPULATION_AND_VEHICLES.md
  UNDERGROUND_BLACKLIST.md
  FRONTEND_UI_RESEARCH.md
.github/workflows/     CI build
```

## Building

The project uses pinned versions of `nfsmw-2005-sdk`, `MWSDK`, and `NFSPluginSDK`; each serves a different validated layer described in `docs/ENGINE_INTEGRATION_MAP.md`.

Requirements:

- CMake 3.20+
- Visual Studio 2019/2022 with Desktop C++ workload
- Win32/x86 target
- Internet access on first configure so CMake can fetch the SDK

From an **x86 Native Tools** prompt:

```bat
cmake -S . -B build -A Win32
cmake --build build --config Release
```

NFSMW is a 32-bit game, so a 64-bit DLL will not load.

The generated `.asi` belongs in:

```text
<Need For Speed Most Wanted>\scripts\
```

## Important safety rule

The project will **not** directly edit or rewrite the career save for pink-slip transfers until a safe, verified engine-backed transaction path exists.

A pink-slip system must be transactional:

```text
validate -> snapshot -> transfer -> verify -> commit
                              \-> rollback on failure
```

See `docs/SAVE_SAFETY.md`.

## External research basis

This project is being built using public clean-room reverse-engineering/modding work, especially:

- https://github.com/s-b-repo/nfsmw-2005-sdk
- https://github.com/s-b-repo/nfsmw-2005-re
- https://github.com/TsyVM/MWSDK
- https://github.com/TsyVM/MWEncyclopedia
- https://github.com/berkayylmao/NFSPluginSDK
- https://github.com/berkayylmao/NFS-Chat-Chaos-Mod

No EA game assets are stored in this repository.


## v0.0.24 motion capture

Opt-in `MotionCaptureEnabled=1` records each raw motion sample with a capture/cohort ID, generation and exact player model key. The observer resets across player/profile/world identity changes, uses at most 120 consecutive accepted pairs and revokes stability on rejected pairs. `tools/analyze_motion_capture.py` independently audits these records; it never verifies metre calibration. See [MOTION_CAPTURE.md](docs/MOTION_CAPTURE.md) for the target-machine procedure and [MOTION_SCALE_CALIBRATION.md](docs/MOTION_SCALE_CALIBRATION.md) for physical-unit research.

## v0.0.25 render observation recovery

A target log proved ASI/executable loading but contained no render/input callbacks or motion captures. The guarded render bridge now checks hook installation, follows device changes and intercepts EndScene/Present method entries, including cached method pointers. Present provides a read-only fallback without duplicate sampling. Periodic health and actual INI switches distinguish installation from callback delivery. See [RENDER_HOOK_RECOVERY.md](docs/RENDER_HOOK_RECOVERY.md). Target render delivery is now confirmed by the supplied v0.0.25 capture; vehicle construction remains blocked. See [target results](docs/TARGET_CAPTURE_2026-10-07.md).

## v0.0.26 gameplay observation and spatial correction

A signature-verified cdecl-float main-loop bridge replaces the unobserved legacy input adapter for the fallback challenge key. It reports genuine loop delivery separately from native input polling, follows entry/completion and revokes callbacks on thread changes. FrameTick ownership remains opt-in; world collision retains thread/world/lease checks. The live OBB adapter now builds valid geometry before validation, correcting unconditional rejection of every box. See [GAMEPLAY_LOOP_RECOVERY.md](docs/GAMEPLAY_LOOP_RECOVERY.md) for the next target test. Vehicle construction remains blocked.
