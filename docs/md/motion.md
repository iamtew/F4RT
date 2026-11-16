# Motion

![Motion](motion.png)

The strip at the top is the hardwired map: each envelope always hits its dest, even with every Matrix row Off. Same colours as the Sound sections they feed.

| Envelope | Always goes to |
|---|---|
| Pitch → osc | pitch (× 0.15) |
| Filter → cutoff | cutoff × **Env amount** on Sound |
| FM → amounts | FM amounts × **FM Env** on Sound |
| Vowel → morph | formant morph (Alienness × LFO 1 also) |
| Grit → drive | grit drive |
| Loudness → amp | VCA. Sustain near zero is a one-shot toot |

The LFOs are Matrix sources (right-click a dest knob on Sound, or wire them on Matrix). Shape, Rate, Amount, **Sync**. LFO colours match Osc 1–4.

Park Loudness sustain up and you get a normal held note, which is allowed, if a little less on-theme.
