# F4RT DSP notes

- One-shot (amp sustain near 0): one toot. Note-off does not chop it. Release time is added to the decay. Size and Rip bake into the envelope at note start.
- Glide slews pitch in log-Hz over the glide time. Mono and legato only, and only when the voice is already sounding. Poly snaps.
- Pitch bend is ± the Bend choice (2, 12, or 24 semitones). Stored wheel is ±1.
- Formant, filter, and distortion run per channel, so osc pan and matrix Pan reach the outputs.
- Osc: sine, triangle, polyBLEP saw/square, noise LCG, 2048-point wavetable.
- FM: previous-sample 4×4, amount scaled by Anger / FM env.
- Formant: three bandpasses, vowels A/E/I/O/U, morph A→B. Key track moves filter cutoff only, not oscillator pitch.
- Delay, flanger, frequency shift, and grain run per channel, so osc pan survives the FX rack.
- Filter: TPT SVF (LP/HP/BP/notch) or comb delay.
- Distortion: tanh, clip, tube, fold, bitcrush, digital.
- Grain FX is a delay-line stutter, not a granular synth engine.
