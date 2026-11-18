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
