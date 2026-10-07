# Frontend UI research — real secondary Blacklist screen

## Conclusion

A real additional frontend screen is technically plausible and is the target.

We do not need to overwrite the vanilla Blacklist's data and pretend it has
extra ranks.

NFSMW's frontend is a hierarchical FNG screen-stack system.

Public reverse engineering identifies:

- `g_pUIRootContext @ 0x91cadc`
- `g_pFrontEndManager @ 0x91cf90`
- `FE_ScreenStackPush @ 0x516e30`
- `FE_LoadAssetScreenByName @ 0x571eb0`
- `FE_UnloadFNGScreen @ 0x5169b0`
- `FE_RecursiveScreenPop @ 0x525a70`
- `FE_InputDispatcher @ 0x549460`
- `FE_PostMessage @ 0x5989b0`
- `FEObject_GetObject @ 0x524850`

The same research explicitly describes the FNG system as able to load named
screen assets.

## Target architecture

```text
UndergroundBlacklist domain state
        |
        v
BlacklistScreenViewModel
        |
        v
FrontendBridge
        |
        +-- load FRR_UndergroundBlacklist.fng
        +-- push screen on FE stack
        +-- bind input
        +-- fill text nodes
        +-- fill portrait/vehicle nodes
        +-- pop/cleanup
```

The FNG is presentation only.

If the screen crashes or is unavailable, progression remains intact.

## Why we are not patching FRONTB now

The current user installation already contains a heavily modified frontend
stack. Replacing FRONTB.LZC wholesale would create unnecessary compatibility
risk.

When the FNG asset is authored, installation should be a targeted patch that
adds/replaces only the Free Roam Rivals assets.

Potential tooling paths researched:

- Binary-based targeted installer
- open-source GlobalLib / nfs-toolbox FEng/FNG editing support
- FEng tooling used by existing MW frontend mods

The final package must back up or patch only required records. No full
FRONTB replacement should be required by design.

## Text/localization

The user's build also has a PT-BR language package.

Therefore the secondary screen should prefer runtime-provided strings from
Free Roam Rivals config/persistence instead of blindly overwriting
LANGUAGES/English.bin.

This avoids destroying custom labels from other mods.

## Portraits

Portraits can be introduced later through the FNG/TPK asset pipeline.

Until then:

- hidden rival -> silhouette/UNKNOWN
- discovered rival without art -> generated placeholder frame
- custom art present -> portrait

The UI must not require a portrait to function.

## Audio

Frontend screen audio routing exists in the game, but custom voice playback
needs a separate verified asset/playback path.

Do not fake support by overwriting unrelated stock voice IDs.

For now the domain reserves stable IntroVoiceAsset/DefeatVoiceAsset/
ThemeAudioAsset keys. The audio bridge will be implemented only after the
playback/streaming lifecycle is verified.

## Compatibility rule

No new frontend hook should ship enabled until it can:

1. verify the supported executable;
2. verify the target FNG asset exists;
3. push and pop the screen repeatedly;
4. restore input/focus after pop;
5. coexist with Widescreen Fix / X360 Stuff / translated frontend;
6. fail closed if any validation fails.
