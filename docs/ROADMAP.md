# Roadmap

## v0.0.1 — Bootstrap

- [x] Repository structure
- [x] Design documentation
- [x] Minimal ASI scaffold
- [x] Configuration templates
- [x] CI build scaffold
- [ ] Confirm ASI build on Win32
- [ ] Confirm plugin loads on target executable
- [ ] Create runtime log in game folder

Exit criterion: loading the ASI does not alter gameplay and creates a diagnostic log.

## v0.1 — One living rival

- [ ] Detect free-roam gameplay state
- [ ] Resolve player vehicle safely
- [ ] Obtain stable per-frame tick
- [ ] Identify/spawn one rival vehicle
- [ ] Keep stable rival handle/pointer validation
- [ ] Rival roaming state
- [ ] Interest distance check
- [ ] ChallengeAvailable state
- [ ] Detect horn or configured fallback input
- [ ] Accept/decline timeout
- [ ] No save writes

Exit criterion: one rival can exist in free roam and naturally enter a challenge-ready state.

## v0.2 — Outrun

- [ ] Implement Outrun race state
- [ ] Determine lead/separation
- [ ] Hold-distance win condition
- [ ] Abort/recovery rules
- [ ] Basic HUD
- [ ] Race result
- [ ] Rival cooldown

Exit criterion: complete repeatable 1v1 Outrun without loading a stock race.

## v0.3 — Staging + cinematic

- [ ] Read road network
- [ ] Find safe staging segment
- [ ] Reserve two poses
- [ ] Rival approach
- [ ] Temporary player control suppression
- [ ] Alignment timeout + hidden snap fallback
- [ ] Real-time intro camera
- [ ] Countdown camera
- [ ] Restore camera/input on every failure path

Exit criterion: accepted challenge transitions into a polished side-by-side start.

## v0.4 — Economy

- [ ] Verified read/write cash bridge
- [ ] Cash stakes
- [ ] Insufficient-money UI
- [ ] Rival virtual cash
- [ ] Reputation stake
- [ ] Transaction framework
- [ ] Never write raw save bytes

Exit criterion: repeatable cash wagers that survive save/load without corruption.

## v0.5 — Rival persistence

- [ ] Rival definitions
- [ ] Personalities
- [ ] W/L history
- [ ] Respect / grudge / fear
- [ ] Persistent rival garage
- [ ] Rematches
- [ ] District preferences
- [ ] Dynamic difficulty without speed cheating

## v0.6 — Pink slip: player wins

- [ ] Verify engine-backed award-car path on supported exe
- [ ] Eligibility rules
- [ ] Last-car rule for rival
- [ ] Add wagered rival car safely
- [ ] Verify garage after transaction
- [ ] Remove car from rival virtual garage only after success
- [ ] Recovery on failure

## v0.7 — Full pink slip

- [ ] Enumerate player-owned cars safely
- [ ] Last-car protection
- [ ] Identify exact wagered car record
- [ ] Snapshot exact customization
- [ ] Remove/transfer using verified engine path
- [ ] Select fallback player car
- [ ] Rival stores won car
- [ ] Recovery race
- [ ] Full rollback tests

**v0.7 must not ship until save-safety tests pass.**

## v0.8 — Living ecosystem

- [ ] Rival uses cars won from player
- [ ] Rival economy
- [ ] Rival upgrades / vehicle changes
- [ ] Rival actively seeks rematch
- [ ] Street value balancing
- [ ] Mixed car + cash wagers

## v0.9 — Rockport systems

- [ ] Police personalities
- [ ] Pursuit continuation
- [ ] Rival may abort due to police
- [ ] Rival-vs-rival races
- [ ] Street Meets
- [ ] Rare / legendary rivals
- [ ] World event scheduler

## v1.0 — Rockport Underground

Release goals:

- stable on supported executables
- no raw save editing
- persistent rivals
- multiple race modes
- polished staging
- cash and pink-slip wagers
- police integration
- safe rollback
- compatibility documentation
