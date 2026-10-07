# Underground Blacklist

## Purpose

The vanilla Blacklist remains untouched.

After the player completes the original career, Free Roam Rivals unlocks a
**second, independent ranked ladder** backed by the mod's own persistent data.

This is not a relabel of the vanilla Blacklist.

The new ladder has its own:

- ranked rivals
- discovery state
- qualification requirements
- world encounters
- win/defeat progression
- portrait/audio asset keys
- future frontend screen

The working internal name is **Underground Blacklist**. The final public name
can change without changing stable rival IDs.

## Unlock

The ladder is unavailable before:

```text
UserProfile::mCareerModeHasBeenCompletedAtLeastOnce == true
```

Once the supported runtime bridge confirms that flag:

```text
Original Blacklist complete
-> Rockport Legend active
-> Underground Blacklist unlocked
-> Rank 10 becomes current target
```

The original game's career data is not rewritten to fake additional ranks.

## Organic progression

The menu does not directly start a boss race.

For the current rank:

```text
Locked/Rumored
-> meet Street Rep + qualifier requirements
-> target becomes eligible to appear in the world
-> player physically finds/sights the rival
-> identity is revealed
-> player approaches in Free Roam
-> horn challenge
-> cinematic staging
-> showdown
-> result persists
-> next rank unlocks
```

This preserves the central Free Roam Rivals fantasy: the ranked opponent is a
real participant in Rockport, not a menu teleport.

## Entry states

### Locked

Future rank or the entire ladder is still locked.

### Rumored

The rank is current, but qualification requirements are incomplete.

The screen may show a silhouette, district rumor and requirement progress.

### HuntAvailable

Requirements are complete, but the player has not yet sighted the target.

The World Director is allowed to schedule/spawn that specific target.

### Discovered

The target has been sighted at least once. Name/portrait may be revealed and
the screen can show richer information.

The player must still encounter the target again in the world to challenge.

### ChallengeReady

The current target is physically present and the encounter system says the
challenge can be offered.

### Defeated

The result is persistent. The next lower-numbered rank becomes current.

## Identity reveal

The progression data stores a separate discovered bit per rank.

This lets the eventual screen do something more interesting than immediately
showing every face:

```text
UNKNOWN / silhouette
-> first sighting
-> reveal name + portrait
-> defeat
-> full stats/history
```

Portraits are therefore presentation assets, not progression truth.

## Qualifiers

The default ladder uses Street Rep plus a small number of ordinary Free Roam
Rival wins before higher ranks become huntable.

Pink-slip wins are NOT required in the initial defaults because the destructive
garage-transfer path is not yet verified. The schema already supports such a
requirement later.

Qualifier wins reset on rank advance.

## Cars

Underground Blacklist opponents are authored/persistent rivals.

Their car must not reroll every time they appear.

They use the same persistent-garage rules as the rest of Free Roam Rivals:

- active car belongs to their garage
- upgrades may persist
- cars won/lost through future Pink Slips modify that garage
- a rival may later drive a car won from the player

## Screen target

Long-term native screen name:

```text
FRR_UndergroundBlacklist.fng
```

The UI is a view over `UndergroundBlacklistSnapshot`. It must never own the
progression logic itself.

That means the same progression remains valid if we temporarily use a debug
overlay before the final FNG asset exists.

## Media pipeline

Each rank already reserves stable presentation keys:

- PortraitAsset
- IntroVoiceAsset
- DefeatVoiceAsset
- ThemeAudioAsset

They are blank by default.

When custom photos/illustrations and audio are ready, adding those assets must
not require changing the saved rival identity or rank progression.

## Initial ranks

The default config contains 10 provisional ranks, #10 through #1.

Names are placeholders and may be redesigned when character art, voices,
cars and personalities are authored.

Stable keys `ub10` ... `ub01` are the persistence identifiers.


## Mod-side persistence

v0.0.10 adds the first real persistence layer for this ladder without touching
the NFSMW save file.

The game profile name is read only long enough to derive a namespaced 64-bit
pseudonymous key in memory. The raw name is not written to disk by the mod.

Files live under:

```text
scripts/FreeRoamRivals/Saves/
  profile_<16-hex-hash>.json
```

The JSON schema currently stores only Free Roam Rivals-owned progression:

```json
{
  "schema": 1,
  "streetRep": 0,
  "qualifierWinsCurrentRank": 0,
  "pinkSlipWins": 0,
  "defeatedMask": 0,
  "discoveredMask": 0
}
```

Career completion is intentionally not persisted here because the game remains
authoritative for that fact. Likewise, `currentTargetPresent` is runtime-only
and is recomputed from the living-world system.

Writes use a temporary file followed by a replace operation. Unsupported or
malformed schemas fail closed instead of silently resetting the player's mod
progress.

The remaining integration step is event-driven mutation: sighting a target,
winning qualifier races and defeating a rank must update the in-memory record
and then commit the new JSON atomically.
