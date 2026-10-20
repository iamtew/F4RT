# F4RT manual

MIDI note-on starts a voice. Pitch follows the note. MPE channel pitch-bend and channel pressure are read. Mod wheel is a matrix source.

Hover any control for a short explanation.

## Top bar

Same layout as the sibling plugins (K1X, 5HKR, TEW03), left to right:

- **Logo / F4RT / version** — click for About (GitHub tag check).
- **Page bar** — `<` `>` step through the pages, click the name for the page list.
- **Patch bar** — `<` `>` step through all patches (200 factory, then your saved ones). Click the name for the menu: Init, Save, Save As, Export, Import, every factory category, and User patches.
- **Bank bar** — the current category. `<` `>` jump to the first patch of the previous / next category, click for the category list.
- **Volume strip** — drag (or double-click to reset) the Level fader; the right half is the output spectrum. Both flash yellow when the output clips.

Saved patches go to `%APPDATA%\Stupid Systems LLC\F4RT\Patches` as `.f4rtp`. Loading a factory patch starts from Init, so FX and matrix settings from the previous patch are cleared.

## Pages

- **Play** — macros, poly mode, voices, rand mode, randomize buttons, Play (test note), wavetable import, Apply FM recipe.
- **Osc** — one row per oscillator (type, vol, pan, coarse, fine, phase, width, FM into osc 1–4), FM env depth and recipe.
- **Filter** — formants, filter, distortion.
- **Motion** — 6 envelopes and 4 LFOs (shape, rate, amount, tempo sync).
- **Matrix** — first 8 of 32 routes (On, src, dest, amount). A route does nothing until its **On** switch is lit. Remaining slots exist as automatable params.
- **FX** — EQ, comp, delay, reverb, chorus, phaser, flanger, freq shift, grain delay. Each effect has its own **On** switch; the knobs do nothing while it is off. Slot combos set order.
- **Browser** — 200 factory seeds, search/tags, favorites (`*`), user `.f4rtp` save/import.
- **Viz** — wave, spectrum, spectrogram, FM graph, envelopes, active matrix routes.

## Macros

Size, Wetness, Anger, Alienness, Squelch, Rip Length, Toxicity. They bake into decay, resonance, FM, formant motion, comb, release, and chaos.

## Random

Modes: Clean, Psytrance, Forest, Darkpsy, Hi-Tech, Experimental. Buttons: All, Safe, FX, Mutate.
