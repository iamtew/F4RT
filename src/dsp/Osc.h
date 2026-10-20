#pragma once

#include <cmath>
#include <cstdint>

namespace f4rt
{

inline constexpr int kWtN = 2048;
inline constexpr float kPi = 3.14159265358979323846f;
inline constexpr float kTwoPi = 6.28318530717958647692f;

inline float midiNoteToHz (float note)
{
    return 440.f * std::pow (2.f, (note - 69.f) / 12.f);
}

inline float polyBlep (float t, float dt)
{
    if (dt <= 0.f)
        return 0.f;
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.f;
    }
    if (t > 1.f - dt)
    {
        t = (t - 1.f) / dt;
        return t * t + t + t + 1.f;
    }
    return 0.f;
}

enum OscType
{
    oscSine = 0,
    oscTri,
    oscSaw,
    oscSquare,
    oscWavetable,
    oscNoise,
    oscUserWt
};

struct Osc
{
    float phase = 0.f;
    float phaseR = 0.5f;
    uint32_t rng = 1;

    void reset (float start, float width)
    {
        phase = start;
        phaseR = start + width * 0.02f;
        if (phaseR >= 1.f)
            phaseR -= 1.f;
        rng = 1u + (uint32_t) (start * 65535.f);
    }

    static float lookup (const float* wt, float ph)
    {
        const float x = ph * (float) kWtN;
        const int i = (int) x & (kWtN - 1);
        const int j = (i + 1) & (kWtN - 1);
        const float f = x - std::floor (x);
        return wt[i] + (wt[j] - wt[i]) * f;
    }

    float tick (int type, float inc, const float* builtIn, const float* user)
    {
        if (inc < 0.f)
            inc = 0.f;
        if (inc > 0.49f)
            inc = 0.49f;

        float s = 0.f;
        switch (type)
        {
            case oscSine:
                s = std::sin (phase * kTwoPi);
                break;
            case oscTri:
            {
                const float t = phase < 0.5f ? phase * 4.f - 1.f : 3.f - phase * 4.f;
                s = t;
                break;
            }
            case oscSaw:
                s = 2.f * phase - 1.f;
                s -= polyBlep (phase, inc);
                break;
            case oscSquare:
                s = phase < 0.5f ? 1.f : -1.f;
                s += polyBlep (phase, inc);
                {
                    float h = phase + 0.5f;
                    if (h >= 1.f)
                        h -= 1.f;
                    s -= polyBlep (h, inc);
                }
                break;
            case oscWavetable:
                s = lookup (builtIn, phase);
                break;
            case oscUserWt:
                s = lookup (user != nullptr ? user : builtIn, phase);
                break;
            default: // noise
                rng = rng * 1664525u + 1013904223u;
                s = ((rng >> 8) / 8388608.f) - 1.f;
                break;
        }

        phase += inc;
        if (phase >= 1.f)
            phase -= 1.f;
        return s;
    }

    float tickStereo (int type, float inc, float width, const float* builtIn, const float* user, float& rOut)
    {
        const float l = tick (type, inc, builtIn, user);
        const float incR = inc * (1.f + width * 0.01f);
        float s = 0.f;
        switch (type)
        {
            case oscSine:
                s = std::sin (phaseR * kTwoPi);
                break;
            case oscTri:
                s = phaseR < 0.5f ? phaseR * 4.f - 1.f : 3.f - phaseR * 4.f;
                break;
            case oscSaw:
                s = 2.f * phaseR - 1.f;
                s -= polyBlep (phaseR, incR);
                break;
            case oscSquare:
                s = phaseR < 0.5f ? 1.f : -1.f;
                s += polyBlep (phaseR, incR);
                {
                    float h = phaseR + 0.5f;
                    if (h >= 1.f)
                        h -= 1.f;
                    s -= polyBlep (h, incR);
                }
                break;
            case oscWavetable:
                s = lookup (builtIn, phaseR);
                break;
            case oscUserWt:
                s = lookup (user != nullptr ? user : builtIn, phaseR);
                break;
            default:
                s = l;
                break;
        }
        phaseR += incR;
        if (phaseR >= 1.f)
            phaseR -= 1.f;
        rOut = s;
        return l;
    }
};

inline void makeBuiltinWt (float* wt)
{
    for (int i = 0; i < kWtN; ++i)
    {
        const float t = (float) i / (float) kWtN;
        // Formant-ish: odd harmonics falling.
        float s = 0.f;
        for (int h = 1; h <= 7; h += 2)
            s += std::sin (t * kTwoPi * (float) h) / (float) h;
        wt[i] = s * 0.6f;
    }
}

} // namespace f4rt
