# F4RT DSP notes

- One-shot (amp sustain near 0): one toot. Note-off does not chop it. Release time is added to the decay. Size and Rip bake into the envelope at note start.
- Osc: sine, triangle, polyBLEP saw/square, noise LCG, 2048-point wavetable.
- FM: previous-sample 4×4, amount scaled by Anger / FM env.
- Formant: three bandpasses, vowels A/E/I/O/U, morph A→B, key track.
- Filter: TPT SVF (LP/HP/BP/notch) or comb delay.
- Distortion: tanh, clip, tube, fold, bitcrush, digital.
- Grain FX is a delay-line stutter, not a granular synth engine.
