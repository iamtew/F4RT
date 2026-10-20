#pragma once

#include <cmath>
#include <cstdint>

namespace f4rt
{

inline constexpr int kLfoDrawN = 32;

enum LfoShape
{
    lfoSine = 0,
    lfoTri,
    lfoRamp,
    lfoRandom,
    lfoSH,
    lfoUser
};

struct Lfo
{
    float phase = 0.f;
    float held = 0.f;
    uint32_t rng = 1;

    void reset (uint32_t seed)
    {
        phase = 0.f;
        held = 0.f;
        rng = seed | 1u;
    }

    float uni()
    {
        rng = rng * 1664525u + 1013904223u;
        return (rng >> 8) / 16777215.f;
    }

    float process (int shape, float inc, const float* draw)
    {
        if (inc < 0.f)
            inc = 0.f;
        const float p0 = phase;
        phase += inc;
        const bool wrap = phase >= 1.f;
        if (wrap)
            phase -= std::floor (phase);

        switch (shape)
        {
            case lfoTri:
                return p0 < 0.5f ? p0 * 4.f - 1.f : 3.f - p0 * 4.f;
            case lfoRamp:
                return p0 * 2.f - 1.f;
            case lfoRandom:
                return uni() * 2.f - 1.f;
            case lfoSH:
                if (wrap || p0 == 0.f)
                    held = uni() * 2.f - 1.f;
                return held;
            case lfoUser:
            {
                if (draw == nullptr)
                    return 0.f;
                const float x = p0 * (float) kLfoDrawN;
                const int i = (int) x & (kLfoDrawN - 1);
                const int j = (i + 1) & (kLfoDrawN - 1);
                const float f = x - std::floor (x);
                return draw[i] + (draw[j] - draw[i]) * f;
            }
            default:
                return std::sin (p0 * 6.28318530718f);
        }
    }
};

} // namespace f4rt
