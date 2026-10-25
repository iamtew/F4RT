#pragma once

#include "Randomizer.h"

#include <cstdio>
#include <cstring>

namespace f4rt
{

inline constexpr int kFactoryCount = 200;

inline const char* kCats[10] = {
    "Classic Farts", "Wet Farts", "Acid Farts", "Croaks", "Burps",
    "Chirps", "Growls", "Zaps", "Lasers", "FX"
};

inline const char* kCatTag[10] = {
    "classic", "wet", "acid", "croak", "burp", "chirp", "growl", "zap", "laser", "fx"
};

inline const int kCatMode[10] = {
    randClean, randPsy, randPsy, randForest, randForest,
    randHiTech, randClean, randPsy, randExp, randDark
};

struct FactoryEntry
{
    int index;
    const char* cat;
    const char* tags;
    uint32_t seed;
    int mode;
};

inline FactoryEntry factoryAt (int i)
{
    if (i < 0)
        i = 0;
    if (i >= kFactoryCount)
        i = kFactoryCount - 1;
    const int cat = i / 20;
    FactoryEntry e;
    e.index = i;
    e.cat = kCats[cat];
    e.tags = kCatTag[cat];
    e.seed = 0xF4A70000u + (uint32_t) i * 2654435761u;
    e.mode = kCatMode[cat];
    return e;
}

inline void factoryName (int i, char* out, int n)
{
    const auto e = factoryAt (i);
    std::snprintf (out, (size_t) n, "%s %02d", e.cat, (i % 20) + 1);
}

inline void loadFactory (Params& p, int i)
{
    const auto e = factoryAt (i);
    fillParams (p, e.seed, e.mode, randAll, i / 20);
}

} // namespace f4rt
