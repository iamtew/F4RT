# Hello, toot

F4RT is PsyFart Designer: a MIDI instrument for synthetic psytrance farts, squelches, croaks, zaps, and formant hits. You play a note, a voice starts, and the pitch follows. It is not subtle, and that is the point.

Four oscillators, a 4×4 FM grid, vowel formants, character macros, a randomizer with flavours, two hundred factory seeds, and an FX rack. MPE channel pitch-bend and channel pressure are read. Mod wheel is a matrix source. Hover any control for a short explanation.

This is the user book. Internals live under Spec, Architecture, and DSP notes in the sidebar, if you enjoy reading the wiring.

VST3, Audio Unit on macOS, and standalone. Windows, Linux, and Mac.

## Install

The Windows installer is the easy path: `F4RT-{version}-win-x64-installer.msi` (0.0.5 shipped as `F4RT-{version}-win-x64.msi`). Pick VST3, Standalone, or both. Defaults are `Common Files\VST3` and `Program Files\Stupid Audio\F4RT`; each folder can be changed. The standalone install adds a Start menu shortcut.

Or unzip `F4RT-{version}-win-x64-vst3.zip` into your VST3 folder, and/or unzip the standalone zip wherever you like. Same idea on Linux and Mac, minus the installer. Mac also has an Audio Unit zip if that is how you live.

In a DAW, scan for VST3 (or AU) and load **F4RT**. In Standalone, pick your audio device and MIDI keyboard under **Options > Audio/MIDI Settings**.

Mac builds are ad-hoc signed, so Gatekeeper will sulk until you right-click Open the first time (or `xattr -dr com.apple.quarantine` on the `.vst3`, the `.component`, and the `.app`). Linux wants the usual system ALSA, FreeType, fontconfig, and X11 libraries.

## How it makes sound

MIDI note-on starts a voice. Pitch follows the note. A tap — amp sustain near zero, including the Play button — finishes that one note even when note-off arrives in the same block. Amp sustain up is a normal held note.

Glide is the slide time for mono and legato. Zero snaps. Poly ignores it. Bend is the pitch-bend range: 2, 12, or 24 semitones. Osc pan is stereo through the formant, filter, and distortion.

## The window

![Sound page](play.png)

The top bar is always there. Under it sit five tabs:

| Tab | What it is |
|---|---|
| **Sound** | Oscillators, FM, vowel, filter, grit, character, voice, scope |
| **Motion** | Six envelopes and four LFOs |
| **Matrix** | Modulation cables |
| **FX** | The rack and the order |
| **Patches** | Factory seeds and your saves |

Saved patches go to `%APPDATA%\Stupid Audio\F4RT\Patches` as `.f4rtp`.
