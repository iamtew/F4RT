#pragma once

#include <cmath>

namespace f4rt
{

// Three parallel bandpasses. Vowel tables are F1 F2 F3 Hz.
inline const float kVowelHz[5][3] = {
    { 800.f, 1150.f, 2800.f }, // A
    { 400.f, 2000.f, 2800.f }, // E
    { 350.f, 2500.f, 3500.f }, // I
    { 450.f,  800.f, 2800.f }, // O
    { 350.f,  600.f, 2700.f }  // U
};

struct Bandpass
{
    float b0 = 0, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

    void set (float sr, float hz, float q)
    {
        if (hz < 20.f)
            hz = 20.f;
        if (hz > sr * 0.45f)
            hz = sr * 0.45f;
        if (q < 0.3f)
            q = 0.3f;
        const float w = 6.28318530718f * hz / sr;
        const float alpha = std::sin (w) / (2.f * q);
        const float c = std::cos (w);
        const float a0 = 1.f + alpha;
        b0 = alpha / a0;
        b1 = 0.f;
        b2 = -alpha / a0;
        a1 = -2.f * c / a0;
        a2 = (1.f - alpha) / a0;
    }

    float process (float x)
    {
        const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
        x2 = x1;
        x1 = x;
        y2 = y1;
        y1 = y;
        return y;
    }

    void reset() { x1 = x2 = y1 = y2 = 0.f; }
};

struct Formant
{
    Bandpass bp[3];

    void set (float sr, int vowelA, int vowelB, float morph, float track, float noteHz)
    {
        const float t = morph < 0.f ? 0.f : (morph > 1.f ? 1.f : morph);
        const int a = vowelA < 0 ? 0 : (vowelA > 4 ? 4 : vowelA);
        const int b = vowelB < 0 ? 0 : (vowelB > 4 ? 4 : vowelB);
        const float ratio = noteHz > 1.f ? noteHz / 261.63f : 1.f;
        const float tr = std::pow (ratio, track);
        for (int i = 0; i < 3; ++i)
        {
            const float hz = (kVowelHz[a][i] + (kVowelHz[b][i] - kVowelHz[a][i]) * t) * tr;
            bp[i].set (sr, hz, 8.f);
        }
    }

    float process (float x)
    {
        return (bp[0].process (x) + bp[1].process (x) + bp[2].process (x)) * 0.55f;
    }

    void reset()
    {
        for (auto& f : bp)
            f.reset();
    }
};

} // namespace f4rt
