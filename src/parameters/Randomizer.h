#pragma once

#include "../dsp/Voice.h"

#include <cstdint>

namespace f4rt
{

enum RandMode
{
    randClean = 0,
    randPsy,
    randForest,
    randDark,
    randHiTech,
    randExp
};

enum RandKind
{
    randAll = 0,
    randSafe,
    randFxOnly,
    randMutate
};

struct Rng
{
    uint32_t s;
    explicit Rng (uint32_t seed) : s (seed | 1u) {}
    uint32_t u()
    {
        s = s * 1664525u + 1013904223u;
        return s;
    }
    float uni() { return (u() >> 8) / 16777215.f; }
    float range (float a, float b) { return a + (b - a) * uni(); }
    int irange (int a, int b) { return a + (int) (u() % (uint32_t) (b - a + 1)); }
};

inline void applyFmPreset (Params& p, int which)
{
    for (int i = 0; i < kNumOsc; ++i)
        for (int j = 0; j < kNumOsc; ++j)
            p.fm[i][j] = 0.f;
    switch (which)
    {
        case 1: // metallic bark
            p.fm[1][0] = 0.55f;
            p.fm[2][0] = 0.2f;
            p.osc[1].type = oscSquare;
            p.osc[1].vol = 0.15f;
            p.osc[1].coarse = 19.f;
            break;
        case 2: // alien chirp
            p.fm[1][0] = 0.7f;
            p.fm[2][1] = 0.35f;
            p.osc[1].type = oscSine;
            p.osc[1].vol = 0.1f;
            p.osc[1].coarse = 24.f;
            break;
        case 3: // growl
            p.fm[1][0] = 0.4f;
            p.fm[0][1] = 0.15f;
            p.osc[1].type = oscSaw;
            p.osc[1].vol = 0.2f;
            p.osc[1].coarse = -12.f;
            break;
        default: // simple FM fart
            p.fm[1][0] = 0.35f;
            p.osc[1].type = oscSine;
            p.osc[1].vol = 0.12f;
            p.osc[1].coarse = 12.f;
            break;
    }
}

inline void fillParams (Params& p, uint32_t seed, int mode, int kind)
{
    Rng rng (seed);
    if (kind == randMutate)
    {
        p.cutoff *= rng.range (0.7f, 1.4f);
        p.res += rng.range (-0.15f, 0.2f);
        p.macros[2] += rng.range (-0.1f, 0.15f);
        p.macros[6] += rng.range (-0.05f, 0.12f);
        p.formMorph = rng.uni();
        return;
    }
    if (kind == randFxOnly)
        return;

    p = defaultParams();
    const bool safe = kind == randSafe;
    p.osc[0].type = safe ? oscSine : rng.irange (0, 4);
    p.osc[0].vol = rng.range (0.6f, 0.95f);
    p.osc[0].coarse = (float) rng.irange (safe ? -5 : -24, safe ? 7 : 19);
    p.osc[1].vol = rng.range (0.f, safe ? 0.2f : 0.5f);
    p.osc[1].type = rng.irange (0, 3);
    p.osc[1].coarse = (float) rng.irange (7, 24);
    if (! safe)
    {
        p.osc[2].vol = rng.range (0.f, 0.3f);
        p.osc[2].type = oscNoise;
        p.osc[3].vol = rng.range (0.f, 0.2f);
        p.osc[3].type = oscWavetable;
    }
    applyFmPreset (p, mode == randClean ? 0 : rng.irange (0, 3));

    p.formA = rng.irange (0, 4);
    p.formB = rng.irange (0, 4);
    p.formMorph = rng.uni();
    p.formMix = mode == randForest ? rng.range (0.3f, 0.8f) : rng.range (0.f, 0.6f);
    p.formTrack = rng.range (0.f, 0.6f);

    p.filtType = mode == randPsy ? filtLP : rng.irange (0, 5);
    p.cutoff = rng.range (200.f, mode == randDark ? 900.f : 5000.f);
    p.res = rng.range (0.15f, mode == randPsy ? 0.85f : 0.55f);
    p.filtEnv = rng.range (0.2f, 0.9f);
    p.distType = mode == randHiTech ? distCrush : (safe ? distSoft : rng.irange (0, 5));
    p.distDrive = rng.range (0.05f, safe ? 0.3f : 0.8f);
    p.distMix = rng.range (0.1f, 0.7f);

    p.env[5].a = rng.range (0.0005f, 0.02f);
    p.env[5].d = rng.range (0.05f, 0.4f);
    p.env[5].r = rng.range (0.05f, 0.6f);
    p.env[1].d = rng.range (0.04f, 0.3f);
    p.macros[0] = rng.range (0.2f, 0.8f);
    p.macros[1] = rng.range (0.1f, 0.9f);
    p.macros[2] = rng.range (0.f, mode == randExp ? 0.9f : 0.55f);
    p.macros[3] = rng.range (0.f, 0.7f);
    p.macros[4] = rng.range (0.1f, 0.8f);
    p.macros[5] = rng.range (0.15f, 0.85f);
    p.macros[6] = rng.range (0.f, mode == randDark ? 0.8f : 0.35f);
    p.lfo[0].rateHz = rng.range (0.5f, 12.f);
    p.lfo[0].amt = rng.range (0.f, 0.6f);
    p.lfo[0].shape = mode == randHiTech ? lfoSH : rng.irange (0, 4);
}

} // namespace f4rt
