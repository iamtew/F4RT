# F4RT manual

MIDI note-on starts a voice. Pitch follows the note. MPE channel pitch-bend and channel pressure are read. Mod wheel is a matrix source.

Hover any control for a short explanation.

## Top bar

Left to right:

- **Logo / F4RT / version** — click for About (GitHub tag check).
- **Patch bar** — `<` `>` step through all patches (200 factory, then your saved ones). Click the name for the menu: Init, Save, Save As, Export, Import, every factory category, and User patches.
- **Bank bar** — the current category. `<` `>` jump to the first patch of the previous / next category, click for the category list.
- **Volume strip** — drag (or double-click to reset) the Level fader; the right half is the output spectrum. Both flash yellow when the output clips.

Under that, five tabs: **Sound**, **Motion**, **Matrix**, **FX**, **Patches**.

Saved patches go to `%APPDATA%\Stupid Systems LLC\F4RT\Patches` as `.f4rtp`. Loading a factory patch starts from Init, so FX and matrix settings from the previous patch are cleared.

## Pages

- **Sound** — the chain on one screen: oscillators (who makes the toot), FM (who bends whose pitch), vowel → filter → grit, Character macros, Voice (Play, poly, glide, randomize), and a scope. Import WT sits on the oscillator section. Recipe + Apply FM sit on the FM section; the recipe menu does nothing until you Apply.
- **Motion** — six envelopes (Pitch, Filter, FM, Vowel, Grit, Loudness) with Attack / Decay / Sustain / Release, then four LFOs. Loudness sustain near zero is a one-shot toot.
- **Matrix** — first 16 of 32 cables (On, From, To, Amount). A route does nothing until its **On** switch is lit. Remaining slots exist as automatable params. Source **Mod wheel** is MIDI CC1.
- **FX** — EQ, comp, delay, reverb, chorus, phaser, flanger, ring shift, stutter delay. Each effect has its own **On** switch; the knobs do nothing while it is off. Order combos set the chain (left is first).
- **Patches** — 200 factory seeds, search/tags, favorites (`*`), user `.f4rtp` save/import.

The scope on Sound can show wave, spectrum, spectrogram, FM graph, envelopes, or active matrix routes.

## Macros

Size, Wetness, Anger, Alienness, Squelch, Rip Length, Toxicity. They bake into decay, resonance, FM, formant motion, and release. Size and Rip are read when the note starts. Comb is the filter type, not a Squelch threshold.

A tap — amp sustain near zero, including the Play button — finishes that one note even when note-off arrives in the same block. Amp sustain up is a normal held note.

Glide is the slide time for mono and legato. Zero snaps. Poly ignores it. Bend is the pitch-bend range: 2, 12, or 24 semitones. Osc pan is stereo through the formant, filter, and distortion.

## Random

Modes: Clean, Psytrance, Forest, Darkpsy, Hi-Tech, Experimental. Buttons: All, Safe, FX, Mutate.
