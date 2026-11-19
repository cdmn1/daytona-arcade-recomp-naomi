# TODO

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
