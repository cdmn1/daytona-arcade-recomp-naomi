# TODO

## Audio bank residency
- [ ] Replace the permanently resident `sfx/race.afb` bank with phase-specific attract/credit, course/transmission selection, race, and results banks. Keep the complete audio library in the NetDIMM image/main-RAM cache and upload only the active phase's working set to AICA. Preload the next phase in main RAM and switch banks at safe transitions; preserve effects/voices that intentionally overlap a transition. Add per-phase and transition-overlap AICA budget checks. See [NAOMI audio bank residency plan](docs/naomi-audio-bank-design.md).
  - [x] First runtime slice: `AE2158` is isolated in `sfx/select.*`; selection entry switches from race to select, and race entry waits for the outgoing selection-bank flow to finish before loading `sfx/race.*`. Host coverage exercises the wait.
  - [x] Selection start sequence inventory: the original trace sends `9E11 + AE2001 + AE2112` together at phase 16 (trace frame 4312). `9E11` is the crowd-cheer sample; `AE2001` is the accompanying mixed selection cue. Both are assigned to the select bank, with `9E11` intentionally shared with race. The `g44f85df` Flycast test confirms `AE2001` works but `9E11` remains inaudible despite being present in the generated select bank; investigate command dispatch/activation.
  - [x] Add the course-specific `AE2112`/`AE2113` announcement phrases to the streamed voice pack; both are rendered in `render.tsv` and are issued by the selection state machine, but were absent from the previous voice manifest.
  - [ ] Package a `t03` ADPCM fallback so the largest course recording can load beside the race SFX bank. The SFX builder checks selection-bank residency against `t02` as well as race-bank residency against `t03`.
  - Remaining work: classify the other phase commands and add attract/credit and results banks.
- [ ] Complete the conservative phase inventory in [naomi-audio-phase-inventory.md](docs/naomi-audio-phase-inventory.md): trace every `9E` effect and results command before assigning final bank membership. A host `check` guard now preserves the full effect kit, the shared selection/race crowd cue, the known credit/podium/results callsites, and the `AE2001` selection cue; generated race/results command membership remains unresolved. `9E12` is needed for credits outside races; `AE2158` must survive the select-to-race handoff until its one-shot ends. Do not remove effects based only on short captures.

## Course / transmission selection
- [ ] Identify and restore the missing confirmation SFX that plays simultaneously with the course-selected and transmission-selected voice lines. Trace the original ROM's course confirmation (23b504) and transmission confirmation (23b514 / 23b53c) sound-command sequences; preserve simultaneous playback rather than delaying or replacing the voice.
- [ ] Restore any other missing course/transmission selection sounds after the confirmation SFX is matched against the original.
- [ ] Restore the rotating stop/start sign and its corresponding SFX at the start of the Intermediate and Advanced tracks. Missing since the first native build; likely inherited from the dreamcast-native port.

## Audio / timing
- [ ] After the course/transmission confirmation SFX is fixed, replace the single-active-phrase voice streamer with concurrent voice playback. Preserve original sound-command timing and allow overlapping voice clips; use independent playback instances/resources with safe AICA RAM limits. Compare against original sound traces to verify which events overlap.
- [ ] Fix the Game Over music start timing: it currently begins about 1–2 seconds late relative to the letters appearing. Compare the original game's Game Over sequence and inherited dreamcast-native timing.
- [ ] Add the missing time-expired finishing-position voice (for example, "You placed 20th") and verify the correct position-specific line is selected.
- [ ] Check the Game Over/results presentation: the position and time text should overlay the live game scene as in the original, not appear over a black screen.

## Region-dependent presentation and audio
- [ ] When COUNTRY is set to EXPORT/USA (English), show the English REVERSE sign instead of the Japanese kanji sign while reversing.
- [ ] In EXPORT/USA mode, replace/suppress the Japanese reverse-time bonus/easter-egg voice with the correct English-region behaviour. Verify the intended regional variant against the original ROM.

## Audio
- [ ] Investigate incomplete in-game voice playback inherited from the dreamcast-native port. The Naomi Game Test VOICE sound test works, but only some in-game voice lines currently play.

## Country / region
- [ ] Wire the saved COUNTRY setting into runtime region behaviour:
  - Localize menus and other Japanese text for the selected region.
  - Select the appropriate track-card artwork/content for the selected region.
  - Use MPH for USA and km/h for JPN/EXPORT.
  - Audit any other region-dependent text, audio, and presentation.

## Cabinet type
- [ ] Wire the saved CABINET setting into runtime cabinet behaviour, including the cabinet-specific colour translation tables. Verify DELUXE and UPLIGHT produce visibly different, correct output.

## Select-screen presentation
- [ ] Restore the rotating car models on the transmission / circuit selection screen. This is already missing in the inherited dreamcast-native build, so track it separately from Naomi-specific regressions.
- [ ] Reproduce the selected-card animation: retain the steady card frame while animating the large red selection backing and the flashing inner blue/cyan card area. Use the original-game video reference (00:53–01:05) when implementing and validating the effect.
