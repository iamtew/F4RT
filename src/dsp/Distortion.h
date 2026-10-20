#pragma once

#include <cmath>
#include <cstdint>

namespace f4rt
{

enum DistType
{
    distSoft = 0,
    distHard,
    distTube,
    distFold,
    distCrush,
    distDigital
};

struct Distortion
{
    float lp = 0.f;
    uint32_t hold = 0;
    float held = 0.f;

    void reset()
    {
        lp = 0.f;
        hold = 0;
        held = 0.f;
    }

    float process (int type, float x, float drive, float tone, float mix)
    {
        const float g = 1.f + drive * 12.f;
        float w = x * g;
        switch (type)
        {
            case distHard:
                w = w > 1.f ? 1.f : (w < -1.f ? -1.f : w);
                break;
            case distTube:
                w = std::tanh (w * 1.4f);
                break;
            case distFold:
                w = std::sin (w);
                break;
            case distCrush:
            {
                const float bits = 3.f + (1.f - drive) * 9.f;
                const float step = 2.f / std::pow (2.f, bits);
                w = std::floor (w / step + 0.5f) * step;
                break;
            }
            case distDigital:
                w = w > 0.8f ? 0.8f : (w < -0.8f ? -0.8f : w);
                w = w > 0.f ? 0.8f : (w < 0.f ? -0.8f : 0.f);
                break;
            default:
                w = std::tanh (w);
                break;
        }
        const float a = 0.05f + tone * 0.9f;
        lp += a * (w - lp);
        return x + (lp - x) * mix;
    }
};

} // namespace f4rt
