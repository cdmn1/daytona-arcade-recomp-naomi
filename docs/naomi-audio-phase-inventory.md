# NAOMI audio phase inventory

This is a conservative inventory of command ownership from the current runtime call sites and
the existing ROM-derived sound pack. It is deliberately not a complete final bank manifest:
commands not proven to be phase-exclusive remain in their current allocation until traced.

## Confirmed phase use

| Phase | Command / asset | Evidence | Residency decision |
|---|---|---|---|
| Attract / title / credit | `9E12` (coin/service credit) | `code/practice.c`, paid coin and service-credit handlers | Must be available before and during attract/title; cannot live only in the race bank |
| Course / transmission selection | `AE2158` | Captured original-ROM command trace: next-tick transition cue after course/transmission confirmation | Selection bank candidate; must remain alive until its one-shot finishes |
| Course / transmission selection | `AE215A`, `AE215B`, `AE215C-` and other selection phrases | Current `voices` pack includes selection prompts; `include/select.h` and captured traces identify the selection prompts/confirmations | Keep in main-RAM voice pack; don't duplicate into the race bank |
| Race | `E0-E9` | `prepare_sfx.cpp` generates the ten controlled engine layers; `race_sound.c` updates them from race telemetry | Race-only candidate |
| Race | `9E01-9E2D` | Channel-14 effect kit; upstream call-site evidence confirms crash and prop-hit effects including `9E08`, `9E1B`, `9E27` | Race effects are retained pending full command-use audit; `9E12` is also needed outside race |
| Race | `AE2109-AE210C`, `AE2010` | Existing bank manifest identifies countdown calls and lap crowd cue | Race candidate |
| Results / game over | Result music and result sequence commands | `code/gameplay/results.c` and `code/practice.c` issue result-entry, position and finish commands | Must be traced before assigning a results-only bank |

## Important lifetime constraints

- `AE2158` occurs on the tick after the confirmation command. A phase switch at the selection-to-race handoff must not stop it prematurely.
- `9E12` is used for coin/service credit before a race. Moving the entire `9E` kit into a race-only bank would break credit feedback.
- The voice pack is already cached in SH-4 main RAM and uses temporary AICA allocations. Its directory contains both selection and race phrases; this does not require all phrase samples to remain in AICA RAM.
- Race engine layers are controlled flows that must remain resident throughout the race, but they need not be resident in attract or selection.
- Music and SFX have separate lifetimes. Music already loads/releases recordings on demand; bank changes must still respect AICAflow's single-upload/DMA ownership.

## Safe next implementation steps

1. Add a shared/front-end allocation for commands known to run before the race, beginning with `9E12`.
2. Separate the ten engine layers and confirmed race-only flows into a race bank.
3. Keep `AE2158` in a transition-safe selection allocation until its instance ends; only then release that allocation.
4. Trace every `9E` command and the results command schedule before moving remaining effects.
5. Add phase-specific AICA diagnostics and budget checks, including any intentional cross-phase overlap.

Do not mark an effect unused based only on its absence from a short trace. No effect is removed by
this inventory.
