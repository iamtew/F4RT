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
    assert (a.filtType == f4rt::filtLP || a.filtType == f4rt::filtComb);
    assert (a.osc[0].type == f4rt::oscSaw || a.osc[0].type == f4rt::oscSquare);
    assert (a.filtEnv >= 0.7f);
    assert (a.macros[4] <= 0.68f);
    assert (b.filtType == f4rt::filtLP || b.filtType == f4rt::filtComb);
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

    // A tap (note-off after 64 samples) still plays the body. A chopped held note does not.
    // Peaks of fat and bright seeds sit in one band.
    auto energyOf = [] (const f4rt::Params& patch, int n)
    {
        std::vector<float> buf ((size_t) n);
        f4rt::renderMono (buf.data(), n, 44100.0, patch);
        float e = 0.f;
        for (float s : buf)
            e += s * s;
        return e;
    };
    f4rt::Params fat;
    f4rt::loadFactory (fat, 0);
    assert (fat.osc[2].type == f4rt::oscSine);
    assert (fat.osc[2].coarse == -12.f);
    assert (fat.env[5].s == 0.f);
    const int phraseN = 44100 * 4;
    const float ePhrase = energyOf (fat, phraseN);
    f4rt::Params chopped = fat;
    chopped.env[5].s = 0.8f;
    chopped.env[5].a = 0.02f;
    chopped.env[5].r = 0.01f;
    const float eChop = energyOf (chopped, phraseN);
    assert (ePhrase > eChop * 4.f);

    f4rt::Params chirp;
    f4rt::loadFactory (chirp, 100);
    assert (chirp.cutoff >= 900.f);

    const int seeds[] = { 0, 20, 40, 100, 140, 180 };
    for (int index : seeds)
    {
        f4rt::Params patch;
        f4rt::loadFactory (patch, index);
        const float peak = f4rt::phrasePeak (patch);
        assert (peak > 0.25f);
        assert (peak < 1.2f);
    }

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

    // Glide slides a sounding mono/legato note. Poly and zero glide snap.
    {
        f4rt::Params g = f4rt::defaultParams();
        g.polyMode = 1;
        g.glide = 0.1f;
        g.env[5].s = 1.f;
        f4rt::Voice v;
        v.prepare (44100.0);
        f4rt::Matrix mx;
        v.start (f4rt::midiNoteToHz (48.f), 1.f, g);
        const float from = v.noteHz;
        v.glideTo (f4rt::midiNoteToHz (60.f), g);
        assert (std::abs (v.noteHz - from) < 0.1f);
        float l = 0.f, r = 0.f;
        for (int i = 0; i < 2205; ++i)
            v.process (g, mx, 120.f, 0.f, l, r);
        assert (v.noteHz > from + 10.f);
        assert (v.noteHz < f4rt::midiNoteToHz (60.f) - 10.f);
        for (int i = 0; i < 2205; ++i)
            v.process (g, mx, 120.f, 0.f, l, r);
        assert (std::abs (v.noteHz - f4rt::midiNoteToHz (60.f)) < 1.f);
        g.polyMode = 2;
        v.glideTo (f4rt::midiNoteToHz (72.f), g);
        assert (std::abs (v.noteHz - f4rt::midiNoteToHz (72.f)) < 0.1f);
        g.polyMode = 0;
        g.glide = 0.f;
        v.noteHz = from;
        v.glideTo (f4rt::midiNoteToHz (60.f), g);
        assert (std::abs (v.noteHz - f4rt::midiNoteToHz (60.f)) < 0.1f);
    }

    // Hard-right osc pan still comes out of the right channel after the filter.
    {
        f4rt::Params pan = f4rt::defaultParams();
        pan.osc[0].pan = 1.f;
        pan.osc[0].width = 0.f;
        pan.formMix = 0.f;
        pan.distMix = 0.f;
        f4rt::Voice v;
        v.prepare (44100.0);
        f4rt::Matrix mx;
        v.start (f4rt::midiNoteToHz (48.f), 1.f, pan);
        float el = 0.f, er = 0.f;
        for (int i = 0; i < 2000; ++i)
        {
            float l = 0.f, r = 0.f;
            v.process (pan, mx, 120.f, 0.f, l, r);
            el += l * l;
            er += r * r;
        }
        assert (er > el * 20.f);
        assert (er > 0.01f);
    }

    assert (f4rt::bendSemis (0) == 2);
    assert (f4rt::bendSemis (1) == 12);
    assert (f4rt::bendSemis (2) == 24);
    assert (f4rt::bendChoice (2) == 0 && f4rt::bendChoice (24) == 2);

    assert (f4rt::versionNewer ("0.0.2", "0.0.1"));
    assert (! f4rt::versionNewer ("0.0.1", "0.0.1"));
    std::vector<std::string> tags { "0.0.3", "nope", "v0.0.2" };
    assert (f4rt::newestNewerTag (tags, "0.0.1") == "0.0.3");

    std::puts ("fart_check ok");
    return 0;
}
