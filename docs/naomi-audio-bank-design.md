# NAOMI audio bank residency plan

## Goal

Keep the original sound library. Do not solve AICA RAM pressure by deleting valid effects or
reducing the audio set to fit one permanently resident bank.

The NetDIMM image is already resident in NAOMI memory. Asset reads therefore have no
optical-disc seek/rotation constraint. The runtime still has to transfer sample data into
AICA sound RAM before AICA can play it, but it only needs the current phase's working set
(and any deliberately preloaded next phase) in the AICA arena.

## Current implementation

- `sound.c` loads one music recording on demand. When a different track is requested,
  it stops/releases the previous music bank and loads the new recording.
- `sfx.c:nm_sfx_init()` loads `sfx/race.afb` and uploads every flow in
  `sfx/race.dsx` during audio initialisation. That bank remains resident until shutdown.
- The announcer phrase pack is already read/cached in main RAM and uploaded to AICA when
  requested. It is a separate streaming path.
- AICAflow serializes uploads: the music loader calls `nm_sfx_quiesce()` before its
  sample-bank DMA. Phase changes must preserve this ownership rule.

The race bank's static residency is the main avoidable source of pressure. The 2 MB AICA
arena is a hardware limit; NAOMI's larger SH-4 main RAM is not currently being used as a
general cache for all phase-specific SFX banks.

## Intended phase model

| Phase | Bank contents |
|---|---|
| Attract / title / credit | Intro and attract effects, coin/credit sounds, title prompts |
| Course / transmission selection | Selection prompts, confirmation effects (including `AE2158`), selection UI sounds |
| Race | Engine layers, driving/collision/environment effects, countdown and race calls |
| Results / game over | Results and finishing-position effects, results voices and music cues |
| Shared | Only genuinely cross-phase, small, latency-sensitive sounds |

The exact command membership must be derived from ROM command traces and call sites. Do not
classify a sound as unused merely because it did not appear in a short capture. In particular,
keep `AE2158` in the selection phase, `9E27` in the race phase, and the confirmed crash
effects in the race phase.

## Runtime design

1. Generate a separate AFB/DSX pair for each phase. Keep source assets and the complete
   command-to-flow mapping in the NetDIMM image; do not discard flows when they are not in
   the current phase.
2. Read the next bank from the resident image into SH-4 main-RAM staging/cache. Do not make
   an optical-media timing assumption or read a large asset synchronously in the game update.
3. At a safe phase boundary, stop only the outgoing phase's one-shot/loop instances and
   wait for AICAflow to report their release. Do not cut off a sound that the original game
   intentionally carries across the boundary.
4. Release the outgoing phase's AICA bank and flow allocations, then upload the incoming
   phase's sample bank and flows. Keep the existing single-DMA ownership discipline.
5. Where transition latency matters, stage the next bank in main RAM before the boundary.
   Upload it during a loading/transition pause or spread the transfer across idle waits.
6. Keep music-bank lifetime independent from SFX-bank lifetime. The existing music loader
   already switches recordings on demand; it must continue to coordinate its DMA with SFX.
7. Retain the voice pack in main RAM and make its AICA upload/playback resources independent
   per active phrase so overlapping original voice commands can play concurrently.

## Safety and validation requirements

- Never free a flow or sample bank while an AICA instance can still reference it.
- Keep engine layer flows resident for the full race phase; patch their parameters without
  restarting them on every update.
- Preserve original sound-command order and overlap. A bank switch must not delay a voice
  command or silently drop an effect.
- Report per phase: sample-bank bytes, flow-upload bytes, total AICA bytes, largest free
  block, total free bytes, peak concurrent allocation, and any failed allocation.
- Test every transition in Flycast and on hardware: attract → selection → race → results →
  attract, including aborted selection, replay/next race, and game-over paths.
- The build must fail if any single phase exceeds the AICA arena or if a transition's
  required overlap cannot fit. It must not fail merely because the sum of mutually
  exclusive phase banks exceeds the arena.
- Keep all existing effects until trace/call-site evidence proves a command is genuinely
  unreachable. A small safety margin is required for runtime allocations and fragmentation.

## Implementation order

1. Inventory sound commands by phase using ROM call sites and captured traces.
2. Add phase bank generation and validate the generated command directories offline.
3. Add explicit phase-transition hooks and a safe bank lifecycle in `sfx.c`.
4. Add main-RAM preloading and transition diagnostics.
5. Make voice uploads/playback independently concurrent.
6. Remove the old single-resident-race-bank budget assumption and replace it with
   per-phase plus transition-overlap checks.
