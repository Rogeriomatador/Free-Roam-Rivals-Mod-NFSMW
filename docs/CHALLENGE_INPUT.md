# Challenge Input

v0.0.15 adds the first runtime input source that can eventually accept a Free
Roam rival challenge.

## Current verified state

The public MW05 action maps used by this project expose actions such as:

- steering / gas / brake
- camera lookback/change
- HUD engage event
- pause / next song
- frontend buttons

They do **not** currently expose a verified native `HORN` or `HONK` action.

Because of that, Free Roam Rivals does not pretend that
`HUDACTION_ENGAGE_EVENT` is the horn.

The long-term design still targets horn-to-challenge, but v0.0.15 uses a
configurable fallback key until a real horn source is proven.

## Fallback key

Default configuration:

```ini
[Input]
UseHornToChallenge=1
FallbackChallengeKey=0x47
```

`0x47` is the Windows virtual-key code for **G**.

The key parser accepts decimal or `0x...` hexadecimal syntax.

Set:

```ini
FallbackChallengeKey=0
```

to disable fallback input entirely.

## Edge semantics

Input is sampled from the existing game input-poll callback after the engine
refreshes its own bindings.

The fallback probe is read-only. It does not inject or overwrite game input.

A press is edge-triggered:

```text
released -> pressed  => one challenge edge
pressed  -> pressed  => no repeat
pressed  -> released => rearms
released -> pressed  => next challenge edge
```

Holding the key therefore cannot spam multiple accepts/challenges.

## Queue contract

The input callback does not directly start a race.

Instead, a detected edge is pushed into a small bounded atomic queue.

Future runtime flow:

```text
input poll
 -> fallback rising edge
 -> ChallengeInputProbe queue
 -> EncounterDirector consumes one press
 -> only if ChallengeAvailable
 -> Accepted
 -> StagingStateMachine
```

The queue is intentionally bounded so an inactive EncounterDirector cannot
accumulate unbounded presses.

## Horn integration

`UseHornToChallenge=1` currently records the intended design and causes the
runtime to explain in the log that no verified native MW05 horn action has
been mapped yet.

A real horn integration must come from a verified source, for example:

- a genuine hidden controller action;
- a vehicle/audio horn state;
- another engine event that can be tied specifically to honking.

Until such a source is proven, the fallback key is the only active challenge
input.

## Safety

ChallengeInputProbe:

- reads one configured Windows virtual key;
- runs from the existing input-poll callback;
- emits rising edges only;
- never injects input;
- never manipulates player controls;
- never starts an encounter by itself.

The future EncounterDirector remains responsible for checking rival distance,
state, cooldown and world safety before consuming a press.
