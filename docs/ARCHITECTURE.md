# F4RT architecture

Processor owns APVTS, SynthEngine (64 voices), FxRack, Analyser.

Voice is header-only: 4 osc → FM → stereo formant → filter → distortion, 6 ADSR, 4 LFO, 32-slot matrix. Each channel has its own formant, filter, and distortion state. Macros write into working params. Glide slews mono and legato pitch; poly snaps.

Default patch: osc1 sine, others muted, FX off, 8 voices.

Patches: `%APPDATA%\Stupid Systems LLC\F4RT\Patches\*.f4rtp`. Favorites: `favorites.txt`.

Jucer `version=` is the binary version. No CMake.
