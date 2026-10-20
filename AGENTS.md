# F4RT – Cursor Rules

PsyFart Designer: MIDI-triggered psytrance fart / squelch / croak / zap instrument.

Sibling of TEW03, K1X, 5HKR, V4JB.

## Stack
- JUCE (Projucer workflow only – no CMake)
- C++17, real-time safe: no allocations, no locks, no system calls in processBlock
- APVTS for all parameters
- Just + justfile
- JUCE from `%USERPROFILE%\JUCE`

## Layout
src/
 PluginProcessor.h/.cpp
 PluginEditor.h/.cpp
 dsp/ Osc Voice SynthEngine Formant Filter Distortion Env Lfo ModMatrix FxRack Analyser
 parameters/ ParameterIDs ParameterLayout FactoryBank Randomizer
 ui/ Theme VersionCompare
docs/
justfile
F4RT.jucer

## Version
- Jucer `version=` is what the binary and About dialog show.
- Git tags must match that string (`0.0.1`).
- `just build-dist` names the zips and MSI from the jucer version.

## Colors
- #02f53a
- #e4f20f
- #e0e0e0
- #666666

## Do NOT
- Introduce CMake
- Allocate on the audio thread
- Copy commercial plugin code
