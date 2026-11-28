# NAOMI audio phase inventory

This is a conservative inventory of command ownership from the current runtime call sites and
the existing ROM-derived sound pack. It is deliberately not a complete final bank manifest:
commands not proven to be phase-exclusive remain in their current allocation until traced.

## Confirmed phase use

| Phase | Command / asset | Evidence | Residency decision |
|---|---|---|---|
| Attract / title / credit | `9E12` (coin/service credit) | `code/practice.c` lines 132 and 140, paid coin and service-credit handlers (`0x009e127f`) | Must be available before and during attract/title; cannot live only in the race bank |
| Course / transmission selection | `AE2158` | Captured original-ROM command trace: next-tick transition cue after course/transmission confirmation | Selection bank candidate; must remain alive until its one-shot finishes |
| Course / transmission selection | `AE215A`, `AE215B`, `AE215C-` and other selection phrases | Current `voices` pack includes selection prompts; `include/select.h` and captured traces identify the selection prompts/confirmations | Keep in main-RAM voice pack; don't duplicate into the race bank |
| Selection / race | `E0-E9` | `prepare_sfx.cpp` generates the ten controlled engine layers; the start-your-engines sequence and `race_sound.c` drive them through the same controller words | Shared in select and race banks so the engine loop is available before the rolling start |
| Race / shared effect kit | `9E01-9E2D` | `sfx-banks.txt` currently assigns the complete kit to `race`; known crash/prop effects include `9E08`, `9E1B`, `9E27`; `9E12` is directly called before a race | Keep every command assigned for now. The kit is not phase-exclusive because `9E12` is used for credits |
| Race | `AE2109-AE210C`, `AE2010` | Existing bank manifest identifies countdown calls and lap crowd cue | Race candidate |
| Podium | `9E2D` (podium cue) | `code/practice.c`: `0x9e2d4f` when the podium is in phase 28 at timer 10 | Podium/finish transition use; do not classify the whole 9E kit as race-only |
| Results / game over | Result-entry and rank-announcement commands | `code/gameplay/results.c`: `nm_result_entry_command()` varies by result mode; `nm_result_announcement_step()` emits rank/penalty-dependent commands; `result_field_tick()` forwards dynamic `out.command[]` through the results sound gate | Exact command membership is data-driven and not yet fully resolved; keep candidate commands in the existing allocation |
| Results / game over | `AE1002` | `code/gameplay/results.c`, emitted by the result-wait effect bit | Preserve as a results-phase command; it is not an `AE20xx` SFX sequence entry in the current AICA SFX directory |

## Runtime call-site audit (24 November 2026)

- `practice.c` has two direct pre-race credit callsites for `0x009e127f` and a podium cue `0x9e2d4f` in phase 28. The latter is a distinct finish/podium lifetime, not evidence that every `9E` effect belongs to the race.
- Race commands are mostly generated rather than literal: `race_sound.c` emits countdown, award, spin, player/opponent crash, ambient, smoke, skid, RPM and slip commands from the race sound tables. `practice.c` additionally forwards event-tail, pit-crew, prop/cone and slot-machine commands.
- Results commands are also generated: entry commands depend on result mode/rank, timed announcements depend on rank/penalty, and the results field can forward dynamic event commands. The source callsites are now guarded by `tools/check_sfx_phase_inventory.py`, but that test deliberately does not pretend to know the unresolved command values.

## Important lifetime constraints

- `AE2158` occurs on the tick after the confirmation command. A phase switch at the selection-to-race handoff must not stop it prematurely.
- `9E12` is used for coin/service credit before a race. Moving the entire `9E` kit into a race-only bank would break credit feedback.
- The voice pack is already cached in SH-4 main RAM and uses temporary AICA allocations. Its directory contains both selection and race phrases; this does not require all phrase samples to remain in AICA RAM.
- Race engine layers are controlled flows that must remain resident throughout the race. They are also needed during the start-your-engines selection sequence, so E0-E9 are shared by the select and race banks; the phase switch retires the outgoing instances before releasing that bank.
- Music and SFX have separate lifetimes. Music already loads/releases recordings on demand; bank changes must still respect AICAflow's single-upload/DMA ownership.

## Safe next implementation steps

1. Add a shared/front-end allocation for commands known to run before the race, beginning with `9E12`.
2. Separate the ten engine layers and confirmed race-only flows into a race bank.
3. Keep `AE2158` in a transition-safe selection allocation until its instance ends; only then release that allocation.
4. Trace every `9E` command and the results command schedule before moving remaining effects.
5. Resolve the generated race and results command values from complete original-ROM traces, then add phase-specific AICA diagnostics and per-phase/overlap budget checks.

The host build's `check` target runs `tools/check_sfx_phase_inventory.py`. It verifies that the full `9E01-9E2D` kit remains assigned exactly once, that `E0-E9` and `AE2158` are retained, and that the known credit/podium/results callsites remain present. It is a regression guard, not a completed phase classification.

Do not mark an effect unused based only on its absence from a short trace. No effect is removed by
this inventory.
