#pragma once

namespace f4rt
{

// ponytail: 32 routes, grow if a patch actually fills the table.
inline constexpr int kMatrixSlots = 32;
inline constexpr int kNumSources = 21; // 6 env, 4 lfo, vel, at, 7 macros, random
inline constexpr int kSrcEnv0 = 0;
inline constexpr int kSrcLfo0 = 6;
inline constexpr int kSrcVel = 10;
inline constexpr int kSrcAT = 11;
inline constexpr int kSrcMacro0 = 12;
inline constexpr int kSrcRandom = 19;
// 20 unused spare

enum Dest
{
    destNone = 0,
    destPitch,
    destCutoff,
    destRes,
    destFm,
    destFormMorph,
    destDist,
    destAmp,
    destPan,
    destCount
};

struct MatrixRow
{
    int src = 0;
    int dest = destNone;
    float amt = 0.f;
    bool on = false;
};

struct Matrix
{
    MatrixRow row[kMatrixSlots];

    float sum (int dest, const float* sources) const
    {
        float s = 0.f;
        for (int i = 0; i < kMatrixSlots; ++i)
        {
            if (! row[i].on || row[i].dest != dest)
                continue;
            const int src = row[i].src;
            if (src < 0 || src >= kNumSources)
                continue;
            s += sources[src] * row[i].amt;
        }
        return s;
    }
};

} // namespace f4rt
