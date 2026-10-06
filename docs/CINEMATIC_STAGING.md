# Cinematic Staging

## Objective

Accepted challenges should transition naturally from free roam into a race without a loading screen.

The system uses real-time game cameras, the real current vehicles and the current world location.

## Sequence

```text
Challenge accepted
-> search road ahead
-> reserve staging area
-> rival leads player
-> temporary input assistance
-> align both cars
-> camera intro
-> rival card
-> stake negotiation
-> contract confirmation
-> engine rev / final shot
-> 3, 2, 1
-> release control
```

## Staging styles

### Side-by-side street

Default.

Two vehicles align in adjacent lanes.

### Parking-lot faceoff

Used only at authored/safely detected wide areas.

Cars may stop at an angle for a more cinematic negotiation shot.

### Rolling start

No full stop.

The rival matches speed and the countdown begins while moving.

Ideal for highway encounters.

## Camera timeline

The camera director should support a small data-driven timeline:

```text
Shot A: rival front 3/4
Shot B: player rear 3/4
Shot C: side view of both
Shot D: rear chase start
```

Each shot needs:

- duration
- relative target
- offset
- FOV
- interpolation type
- collision/fallback policy

## Parking/alignment fallback

Perfect AI parking on every road is not a launch requirement.

If approach time expires:

1. move camera to hide precise wheel position
2. validate staging transforms are clear
3. safely align the cars
4. settle physics
5. continue cinematic

If transform validation fails, abort staging and return full control.

No blind teleport into traffic or geometry.

## Interruptions

The director must restore player state after:

- rival destroyed
- player vehicle invalidated
- pursuit transition that invalidates staging
- loading transition
- cutscene starts
- player cancels
- road segment becomes unsafe
- internal exception/failed validation

Camera and input restoration are mandatory cleanup paths.
