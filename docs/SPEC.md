# F4RT spec

This file is the product spec.

PsyFart Designer: MIDI-triggered psytrance fart / squelch / croak / zap instrument. Sibling of TEW03, K1X, 5HKR, V4JB. Stupid Audio. MIT.

- A producer can make many distinct psy-fart sounds without extra FX
- V1.5 / V2.0 (AI presets, sample import as a product line, granular synth, resynthesis, macro scripting) are out of this tree

## Product

- VST3 + Standalone
- About + GitHub tag check (title-bar name + version)
- 200 factory seeds plus user patches
- Patches: `%APPDATA%\Stupid Audio\F4RT\Patches\*.f4rtp`. Favorites: `favorites.txt`
- Jucer `version=` is the binary, About dialog, git tag, and `just build-dist` name. Tags match that string (no `v` prefix).

Tabs: Sound, Motion, Matrix, FX, Patches.

MIDI note-on starts a voice. Pitch follows the note. MPE channel pitch-bend and channel pressure are read. Mod wheel is a matrix source.

## Voice

Header-only voice: 4 osc → FM → stereo formant → filter → distortion, 6 ADSR, 4 LFO, 32-slot matrix. Each channel has its own formant, filter, and distortion state.

- Oscillators 1–4: Type, Volume, Pan, Coarse, Fine, Phase, Width. Import WT loads a wav as the user wavetable.
- FM: 4×4 amounts
- Vowel (formant), Filter, Grit
- Character macros bake into decay, resonance, FM, formant motion, and release: Size, Wetness, Anger, Alienness, Squelch, Rip Length, Toxicity
- Voice: Play (C3 preview), Voices, Mode (Poly / mono / legato), Glide, Bend, Flavour
- Default patch: osc1 sine, others muted, FX off, 8 voices. Engine has 64 voices.

Loudness sustain near zero is a one-shot toot. Glide slews mono and legato pitch; poly snaps.

## Motion and matrix

- Six envelopes (Pitch, Filter, FM, Vowel, Grit, Loudness)
- Four LFOs (Shape, Rate, Amount, Sync)
- 32-slot mod matrix (On, From, To, Amount). A route does nothing until On is lit.

## FX

Enable buttons: EQ, Comp, Delay, Reverb, Chorus, Phaser, Flanger, Ring shift, Stutter. Knobs do nothing while that effect is off. Order combos set the chain.

## Random

Flavour: Clean, Psytrance, Forest, Darkpsy, Hi-Tech, Experimental. Buttons: All, Safe, FX, Mutate.

## Do not

- CMake (Projucer only)
- Allocations, locks, or system calls in `processBlock`
- Commercial plugin code
- AI presets, granular synth, resynthesis, or macro scripting in this tree

## Stack

- JUCE, Projucer workflow only
- C++17, APVTS for all parameters
- Just + justfile
- JUCE from `%USERPROFILE%\JUCE`

## UI

- `#02f53a`
- `#e4f20f`
- `#e0e0e0`
- `#666666`

`assets/logo.png` is the Affinity export. Source of truth. Never overwrite. `just icon` derives the Standalone ICO.

## Layout

```
src/
  PluginProcessor.h/.cpp
  PluginEditor.h/.cpp
  dsp/ Osc Voice SynthEngine Formant Filter Distortion Env Lfo ModMatrix FxRack Analyser
  parameters/ ParameterIDs ParameterLayout FactoryBank Randomizer
  ui/ Theme VersionCompare
docs/
justfile
F4RT.jucer
```

README stays small. LICENSE is MIT. `docs/` is the gh-pages site.
