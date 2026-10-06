# Endgame and Progression

## Core rule

Finishing the vanilla career does **not** disable Free Roam Rivals.

Career completion changes the world state from progression-aware street racing into an open-ended endgame called **Rockport Legend**.

The mod therefore has two progression axes:

```text
Vanilla Career Progress
        +
Free Roam Rivals Street Rep
```

Career progress controls what is believable for the current point in the story.

Street Rep controls how the rival ecosystem reacts to the player.

## Street Rep

Street Rep is mod-owned and never replaces vanilla bounty or blacklist progression.

Suggested rank bands:

| Rank | Rep |
|---|---:|
| Unknown | 0 |
| Newcomer | 25 |
| Known | 75 |
| Respected | 150 |
| Notorious | 300 |
| Elite | 500 |
| Legend | 750 |
| Icon | 1000+ |

Rep may continue increasing after Icon. The rank is a presentation tier, not a hard cap.

### Rep sources

Positive:

- win against a rival
- beat a stronger rival
- win a high-stakes race
- win a pink slip
- survive police involvement and still win
- defeat a legendary rival
- recover a previously lost car

Negative or neutral:

- decline ordinary challenge: no penalty
- abandon an accepted race: small penalty
- repeated dirty behavior against a clean rival: relationship change, not necessarily global Rep
- lose: no automatic global Rep loss unless a special contract says so

The system should avoid making the player afraid to race.

## Career-aware rival tiers

Before career completion, the director uses career progress as a ceiling.

Example intent:

```text
Early career  -> mostly Tier 1-2 rivals
Mid career    -> Tier 2-3
Late career   -> Tier 3-5
Completed     -> Tier 1-5 plus legendary pool
```

Street Rep influences *who wants to challenge the player* inside that ceiling.

A high-Rep player in early career can attract unusually confident opponents, but should not be flooded with endgame supercars.

## Rockport Legend endgame

When vanilla career completion is detected:

```text
CareerCompleted = true
-> RockportLegend = active
```

The world director may now use the full rival tier pool.

New systems become eligible:

- legendary rivals
- high-stakes contracts
- stronger revenge encounters
- elite Street Meets
- rival-versus-rival headline races
- district champions
- rare "hunt" encounters where a rival seeks the player
- larger pink-slip stakes
- special rematch chains

The endgame is intentionally endless.

## District reputation

A future layer may track familiarity/reputation separately per district:

```text
Rosewood
Camden
Downtown
Rockport Highway
```

This does not mean territory ownership.

It answers questions such as:

- where is the player most famous?
- which rivals are more likely to seek them there?
- where should a Street Meet spawn?
- which district champion becomes available?

## Rival ecosystem progression

Rivals also progress.

Each persistent rival may have:

- cash
- garage
- active vehicle
- wins/losses
- respect
- grudge
- fear
- risk tolerance
- cooldown
- personal progression tier

A rival may:

- upgrade a current car
- switch cars
- return for revenge
- become temporarily broke
- reduce stakes while rebuilding
- become a district champion
- unlock a rare car after enough success

The player should not be the only entity that changes.

## Legendary rivals

Legendary rivals are condition-based, not random supercars.

Example eligibility:

```text
career completed
AND Street Rep >= 500
AND total rival wins >= 30
AND pink-slip wins >= 3
```

Individual legends may add their own requirements:

- win streak
- district reputation
- pursuit history
- specific rival defeated
- recovered lost vehicle
- high-stakes wins

The encounter itself should remain organic: the player sees the car in the world before a giant UI announcement.

## Long-term world director

The world director schedules opportunities, not scripted missions.

Possible events:

- ordinary roaming rival
- revenge rival
- rival hunting player
- Street Meet
- rival-vs-rival race
- police-heavy "crackdown" period
- district champion appearance
- legendary sighting

The director must use cooldowns so the world feels alive rather than spammy.
