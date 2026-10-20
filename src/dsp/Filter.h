#pragma once

#include <cmath>

namespace f4rt
{

enum FiltType
{
    filtLP = 0,
    filtHP,
    filtBP,
    filtNotch,
    filtComb,
    filtSVF
};

// One TPT-ish state-variable. Comb is a short delay.
struct Filter
{
    float ic1 = 0, ic2 = 0;
    float comb[2048] {};
    int combN = 256;
    int combI = 0;
    float sr = 44100.f;

    void prepare (double sampleRate)
    {
        sr = sampleRate > 1.0 ? (float) sampleRate : 44100.f;
        reset();
    }

    void reset()
    {
        ic1 = ic2 = 0.f;
        combI = 0;
        for (float& s : comb)
            s = 0.f;
    }

    float process (int type, float x, float cutoff, float res, float drive)
    {
        const float d = 1.f + drive * 3.f;
        x = std::tanh (x * d);

        if (type == filtComb)
        {
            const float hz = cutoff < 40.f ? 40.f : cutoff;
            combN = (int) (sr / hz);
            if (combN < 2)
                combN = 2;
            if (combN > 2047)
                combN = 2047;
            const float y = x + comb[combI] * (0.2f + res * 0.78f);
            comb[combI] = y;
            combI = (combI + 1) % combN;
            return y;
        }

        float g = std::tan (3.14159265f * cutoff / sr);
        if (g > 4.f)
            g = 4.f;
        const float k = 2.f - res * 1.85f;
        const float g1 = 1.f / (1.f + g * (g + k));
        const float hp = (x - (g + k) * ic1 - ic2) * g1;
        const float bp = g * hp + ic1;
        const float lp = g * bp + ic2;
        ic1 = g * hp + bp;
        ic2 = g * bp + lp;
        const float notch = hp + lp;

        switch (type)
        {
            case filtHP:
                return hp;
            case filtBP:
                return bp;
            case filtNotch:
                return notch;
            default: // LP and SVF
                return lp;
        }
    }
};

} // namespace f4rt
