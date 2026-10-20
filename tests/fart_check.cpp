#include "dsp/Voice.h"
#include "parameters/FactoryBank.h"
#include "ui/VersionCompare.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

int main()
{
    assert (std::abs (f4rt::midiNoteToHz (69) - 440.f) < 1e-3f);

    f4rt::Params p = f4rt::defaultParams();
    std::vector<float> buf (44100);
    f4rt::renderMono (buf.data(), (int) buf.size(), 44100.0, p);

    float peak = 0.f, energy = 0.f;
    for (float s : buf)
    {
        peak = std::max (peak, std::abs (s));
        energy += s * s;
    }
    assert (peak > 0.01f);
    assert (peak < 8.f);
    assert (energy > 0.5f);

    f4rt::Params a = f4rt::defaultParams();
    f4rt::Params b = f4rt::defaultParams();
    f4rt::fillParams (a, 1, f4rt::randPsy, f4rt::randAll);
    f4rt::fillParams (b, 2, f4rt::randPsy, f4rt::randAll);
    assert (std::abs (a.cutoff - b.cutoff) > 1.f || std::abs (a.macros[2] - b.macros[2]) > 0.01f);
    assert (a.filtType == f4rt::filtLP);
    assert (a.osc[0].type == f4rt::oscSaw || a.osc[0].type == f4rt::oscSquare);
    assert (a.filtEnv >= 0.7f);
    assert (a.macros[4] <= 0.68f);
    assert (b.filtType == f4rt::filtLP);
    assert (b.osc[0].type == f4rt::oscSaw || b.osc[0].type == f4rt::oscSquare);
    assert (b.filtEnv >= 0.7f);
    assert (b.macros[4] <= 0.68f);

    f4rt::applyFmPreset (p, 0);
    const float simple = p.fm[1][0];
    f4rt::applyFmPreset (p, 1);
    assert (p.fm[1][0] != simple || p.osc[1].coarse > 12.f);

    char n0[64], n1[64];
    f4rt::factoryName (0, n0, 64);
    f4rt::factoryName (20, n1, 64);
    assert (std::strcmp (n0, n1) != 0);
    assert (f4rt::kFactoryCount == 200);

    f4rt::Formant f;
    f.set (44100.f, 0, 4, 0.f, 0.f, 261.63f);
    float yA = 0.f, yU = 0.f;
    for (int i = 0; i < 64; ++i)
        yA += std::abs (f.process (std::sin (i * 0.2f)));
    f.reset();
    f.set (44100.f, 4, 4, 0.f, 0.f, 261.63f);
    for (int i = 0; i < 64; ++i)
        yU += std::abs (f.process (std::sin (i * 0.2f)));
    assert (yA > 0.f && yU > 0.f);

    assert (f4rt::versionNewer ("0.0.2", "0.0.1"));
    assert (! f4rt::versionNewer ("0.0.1", "0.0.1"));
    std::vector<std::string> tags { "0.0.3", "nope", "v0.0.2" };
    assert (f4rt::newestNewerTag (tags, "0.0.1") == "0.0.3");

    std::puts ("fart_check ok");
    return 0;
}
