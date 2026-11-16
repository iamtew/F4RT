# Sound

![Sound](play.png)

The chain on one screen: Oscillators, FM, Vowel → Filter → Grit, Character, Voice, and a scope. This is where the fart is born.

- **Oscillators** — Osc 1–4, each in its own colour (same colour on the FM grid row/column). Type, Volume, Pan, Coarse, Fine, Phase, Width. **Import WT** loads a wav as the user wavetable, if you have a waveform you trust more than ours.
- **FM** — 4×4 amounts (row into column). **Recipe** does nothing until **Apply FM**, which is the plugin’s way of not ruining a good grid by accident. **FM Env** scales the amounts (orange, same as the FM envelope on Motion).
- **Vowel** — From, To, Morph, Follow note, Mix. Formants, but make it rude. Colour matches the Vowel envelope. Right-click **Morph** to add a Matrix source.
- **Filter** — Type, Cutoff, Resonance, Drive in, Follow note, Env amount. Green like the Filter envelope. Right-click **Cutoff** or **Resonance** to assign LFO / vel / a macro; Env amount is the hardwired Filter envelope depth, not a Matrix dest.
- **Grit** — Type, Drive, Tone, Mix. Pink like the Grit envelope. Right-click **Drive** to assign extra Matrix sources.
- **Character** — Size, Wetness, Anger, Alienness, Squelch, Rip Length, Toxicity. See [Macros](macros.md) for what they actually bake into.
- **Voice** — **Play** (C3 preview), Voices, Mode (Poly / mono / legato), Glide, Bend, **Flavour**, then All / Safe / FX / Mutate. Flavour and the randomize buttons are covered under [Random](random.md).
- **Scope** — Wave, Spectrum, Spectrogram, FM, Env, Matrix. A picture of the crime.
