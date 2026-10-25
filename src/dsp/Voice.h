#pragma once

#include "Distortion.h"
#include "Env.h"
#include "Filter.h"
#include "Formant.h"
#include "Lfo.h"
#include "ModMatrix.h"
#include "Osc.h"

#include <cmath>

namespace f4rt
{

inline constexpr int kNumOsc = 4;
inline constexpr int kNumEnv = 6; // pitch, filter, fm, formant, dist, amp
inline constexpr int kNumLfo = 4;
inline constexpr int kNumMacro = 7;

struct OscParams
{
    int type = oscSine;
    float vol = 0.f;
    float pan = 0.5f;
    float coarse = 0.f;
    float fine = 0.f;
    float phase = 0.f;
    float width = 0.f;
};

struct EnvParams
{
    float a = 0.005f, d = 0.12f, s = 0.f, r = 0.18f;
};

struct LfoParams
{
    int shape = lfoSine;
    float rateHz = 2.f;
    bool sync = false;
    float amt = 0.f;
};

struct Params
{
    OscParams osc[kNumOsc];
    float fm[kNumOsc][kNumOsc] {};
    float fmEnvDepth = 0.f;
    int formA = 0, formB = 1;
    float formMorph = 0.f, formTrack = 0.f, formMix = 0.f;
    int filtType = filtLP;
    float cutoff = 900.f, res = 0.55f, filtDrive = 0.1f, keyTrack = 0.3f, filtEnv = 0.75f;
    int distType = distSoft;
    float distDrive = 0.15f, distTone = 0.6f, distMix = 0.2f;
    EnvParams env[kNumEnv];
    LfoParams lfo[kNumLfo];
    float lfoDraw[kNumLfo][kLfoDrawN] {};
    float macros[kNumMacro] { 0.45f, 0.55f, 0.2f, 0.1f, 0.5f, 0.4f, 0.05f };
    int polyMode = 2;
    float glide = 0.f; // seconds. 0 snaps. Mono and legato only.
    int bendSt = 2;    // pitch-bend range in semitones: 2, 12 or 24
    int voices = 8;
    float level = 0.7f;
};

// Choice index 0/1/2 is the Bend combo. Anything else stays at 2 st.
inline int bendSemis (int choice)
{
    if (choice == 1)
        return 12;
    if (choice == 2)
        return 24;
    return 2;
}

inline int bendChoice (int semis)
{
    if (semis >= 24)
        return 2;
    if (semis >= 12)
        return 1;
    return 0;
}

inline Params defaultParams()
{
    Params p;
    p.osc[0].vol = 0.85f;
    p.osc[0].type = oscSaw;
    p.env[1].d = 0.08f;
    p.env[5].a = 0.001f;
    p.env[5].d = 0.16f;
    p.env[5].s = 0.f;
    p.env[5].r = 0.22f;
    for (int i = 0; i < kNumLfo; ++i)
        for (int n = 0; n < kLfoDrawN; ++n)
            p.lfoDraw[i][n] = std::sin ((float) n / (float) kLfoDrawN * kTwoPi);
    return p;
}

// Macros bake into working params. Not a second synth.
inline void applyMacros (Params& p)
{
    const float size = p.macros[0];
    const float wet = p.macros[1];
    const float anger = p.macros[2];
    const float squelch = p.macros[4];
    const float rip = p.macros[5];
    const float tox = p.macros[6];

    p.env[5].d *= 0.35f + size * 1.6f;
    p.env[5].r *= 0.25f + rip * 2.2f;
    p.cutoff *= 0.45f + size * 1.1f;
    p.res += wet * 0.35f + squelch * 0.4f;
    if (p.res > 0.98f)
        p.res = 0.98f;
    p.formMix += wet * 0.45f;
    if (p.formMix > 1.f)
        p.formMix = 1.f;
    p.fmEnvDepth += anger * 0.7f;
    p.distDrive += anger * 0.55f;
    if (p.distDrive > 1.f)
        p.distDrive = 1.f;
    if (tox > 0.55f)
        p.distType = distFold;
    for (int i = 0; i < kNumOsc; ++i)
        for (int j = 0; j < kNumOsc; ++j)
            p.fm[i][j] *= 0.15f + anger * 1.5f;
}

struct Voice
{
    Osc osc[kNumOsc];
    Adsr env[kNumEnv];
    Lfo lfo[kNumLfo];
    Formant formant, formantR;
    Filter filter, filterR;
    Distortion dist, distR;
    float lastOsc[kNumOsc] {};
    float sr = 44100.f;
    float noteHz = 261.63f;
    float glideHz = 261.63f;
    float glideStep = 0.f; // log-Hz per sample. 0 means already on the target.
    float vel = 1.f;
    float aftertouch = 0.f;
    float bend = 0.f; // semitones
    float age = 0.f;
    bool gate = false;
    bool oneShot = false; // amp sustain near 0: the note finishes itself
    const float* userWt = nullptr;
    float builtIn[kWtN] {};

    void prepare (double sampleRate)
    {
        sr = sampleRate > 1.0 ? (float) sampleRate : 44100.f;
        filter.prepare (sampleRate);
        filterR.prepare (sampleRate);
        makeBuiltinWt (builtIn);
        reset();
    }

    void reset()
    {
        for (auto& o : osc)
            o.reset (0.f, 0.f);
        for (auto& e : env)
            e.reset();
        for (int i = 0; i < kNumLfo; ++i)
            lfo[i].reset ((uint32_t) (i + 1) * 17u);
        formant.reset();
        formantR.reset();
        filter.reset();
        filterR.reset();
        dist.reset();
        distR.reset();
        for (float& s : lastOsc)
            s = 0.f;
        gate = false;
        oneShot = false;
        age = 0.f;
    }

    // Macros scale decay and release. Bake them here; process() never rewrites envelope times.
    void armEnvs (const Params& baked)
    {
        for (int i = 0; i < kNumEnv; ++i)
        {
            float d = baked.env[i].d;
            float s = baked.env[i].s;
            const float r = baked.env[i].r;
            if (i == 5 && oneShot)
            {
                d += r; // the tail is part of the note, so a fast note-off cannot chop it
                s = 0.f;
            }
            env[i].setTimes (sr, baked.env[i].a, d, s, r);
            env[i].noteOn();
        }
    }

    // Slide only when this voice is already sounding. A fresh note snaps.
    void aim (float hz, const Params& raw, bool slide)
    {
        if (! slide || raw.polyMode == 2 || raw.glide <= 0.0005f || noteHz <= 1.f)
        {
            noteHz = glideHz = hz;
            glideStep = 0.f;
            return;
        }
        glideHz = hz;
        const float samples = raw.glide * sr;
        const float a = std::log (noteHz);
        const float b = std::log (hz > 1.f ? hz : 1.f);
        glideStep = (b - a) / (samples > 1.f ? samples : 1.f);
    }

    // Legato retune. Envelopes stay. Poly and zero glide snap.
    void glideTo (float hz, const Params& raw) { aim (hz, raw, true); }

    void start (float hz, float velocity, const Params& raw)
    {
        aim (hz, raw, env[5].active());
        vel = velocity;
        age = 0.f;
        gate = true;
        Params baked = raw;
        applyMacros (baked);
        oneShot = baked.env[5].s < 0.05f;
        for (int i = 0; i < kNumOsc; ++i)
            osc[i].reset (raw.osc[i].phase, raw.osc[i].width);
        armEnvs (baked);
        for (int i = 0; i < kNumLfo; ++i)
            lfo[i].reset ((uint32_t) (i + 3) * 91u);
        formant.reset();
        formantR.reset();
        filter.reset();
        filterR.reset();
        dist.reset();
        distR.reset();
    }

    void advanceGlide (const Params& p)
    {
        if (p.polyMode == 2 || p.glide <= 0.0005f)
        {
            noteHz = glideHz;
            glideStep = 0.f;
            return;
        }
        if (glideStep == 0.f)
            return;
        const float a = std::log (noteHz > 1.f ? noteHz : 1.f);
        const float b = std::log (glideHz > 1.f ? glideHz : 1.f);
        if ((b - a) * glideStep <= 0.f || std::abs (b - a) <= std::abs (glideStep))
        {
            noteHz = glideHz;
            glideStep = 0.f;
            return;
        }
        noteHz = std::exp (a + glideStep);
    }

    void release()
    {
        gate = false;
        if (oneShot)
            return; // a tap plays the whole note
        for (auto& e : env)
            e.noteOff();
    }

    bool active() const { return env[5].active(); }

    void process (const Params& in, const Matrix& mx, float bpm, float randomSrc, float& l, float& r)
    {
        Params p = in;
        applyMacros (p);
        advanceGlide (p);

        float src[kNumSources] {};
        for (int i = 0; i < kNumEnv; ++i)
            src[kSrcEnv0 + i] = env[i].process();
        for (int i = 0; i < kNumLfo; ++i)
        {
            float inc = p.lfo[i].rateHz / sr;
            if (p.lfo[i].sync && bpm > 1.f)
                inc = (bpm / 60.f) * p.lfo[i].rateHz / sr;
            src[kSrcLfo0 + i] = lfo[i].process (p.lfo[i].shape, inc, p.lfoDraw[i]) * (0.2f + p.lfo[i].amt);
        }
        src[kSrcVel] = vel;
        src[kSrcAT] = aftertouch;
        for (int i = 0; i < kNumMacro; ++i)
            src[kSrcMacro0 + i] = p.macros[i];
        src[kSrcRandom] = randomSrc;

        const float alien = p.macros[3];
        const float tox = p.macros[6];
        const float pitchN = mx.sum (destPitch, src) + src[kSrcEnv0] * 0.15f + bend / 12.f;
        const float cutN = mx.sum (destCutoff, src) + src[kSrcEnv0 + 1] * p.filtEnv;
        const float resN = mx.sum (destRes, src);
        const float fmN = mx.sum (destFm, src) + src[kSrcEnv0 + 2] * p.fmEnvDepth;
        const float morphN = mx.sum (destFormMorph, src) + src[kSrcEnv0 + 3]
                             + src[kSrcLfo0] * alien;
        const float distN = mx.sum (destDist, src) + src[kSrcEnv0 + 4];
        const float ampN = mx.sum (destAmp, src);
        const float panN = mx.sum (destPan, src);

        float left = 0.f, right = 0.f;
        for (int i = 0; i < kNumOsc; ++i)
        {
            if (p.osc[i].vol <= 0.0001f)
                continue;
            float hz = noteHz * std::pow (2.f, (p.osc[i].coarse + p.osc[i].fine / 100.f) / 12.f + pitchN);
            hz *= std::pow (2.f, (p.keyTrack - 0.5f) * 0.1f);
            float fmSum = 0.f;
            for (int j = 0; j < kNumOsc; ++j)
                fmSum += p.fm[j][i] * lastOsc[j];
            hz *= 1.f + (fmSum + fmN) * 4.f;
            if (hz < 8.f)
                hz = 8.f;
            if (hz > sr * 0.45f)
                hz = sr * 0.45f;
            float rS = 0.f;
            const float lS = osc[i].tickStereo (p.osc[i].type, hz / sr, p.osc[i].width,
                                                 builtIn, userWt, rS);
            lastOsc[i] = 0.5f * (lS + rS);
            const float g = p.osc[i].vol;
            const float pan = p.osc[i].pan + panN * 0.2f;
            const float pl = pan < 0.f ? 0.f : (pan > 1.f ? 1.f : pan);
            left += lS * g * (1.f - pl);
            right += rS * g * pl;
        }

        // Osc pan is already in left/right. Each side keeps its own state so that pan
        // is still there after formant, filter and distortion.
        const float mix = p.formMix < 0.f ? 0.f : (p.formMix > 1.f ? 1.f : p.formMix);
        float cut = p.cutoff * std::pow (2.f, cutN * 3.f);
        cut *= std::pow (noteHz / 261.63f, p.keyTrack);
        if (cut < 30.f)
            cut = 30.f;
        if (cut > sr * 0.45f)
            cut = sr * 0.45f;
        float reso = p.res + resN * 0.3f + tox * 0.15f;
        if (reso < 0.f)
            reso = 0.f;
        if (reso > 0.98f)
            reso = 0.98f;
        float drv = p.distDrive + distN * 0.5f;
        if (drv < 0.f)
            drv = 0.f;
        if (drv > 1.f)
            drv = 1.f;
        formant.set (sr, p.formA, p.formB, p.formMorph + morphN, p.formTrack, noteHz);
        formantR.set (sr, p.formA, p.formB, p.formMorph + morphN, p.formTrack, noteHz);
        auto shape = [&] (float x, Formant& form, Filter& filt, Distortion& d)
        {
            const float fv = form.process (x);
            x = x + (fv - x) * mix;
            x = filt.process (p.filtType, x, cut, reso, p.filtDrive);
            return d.process (p.distType, x, drv, p.distTone, p.distMix);
        };
        left = shape (left, formant, filter, dist);
        right = shape (right, formantR, filterR, distR);

        const float amp = src[kSrcEnv0 + 5] * (1.f + ampN) * vel * p.level;
        l = left * amp;
        r = right * amp;
        age += 1.f / sr;
        if (oneShot && env[5].stage == Adsr::Sustain)
            env[5].reset(); // one toot is done; free the voice
    }
};

// Same path as the plugin: macros bake inside the voice, once. A note-off at 64 samples
// is the Play button (on and off in one block). One-shots ignore it.
inline void renderMono (float* dest, int n, double sampleRate, const Params& p)
{
    Voice v;
    v.prepare (sampleRate);
    Matrix mx;
    v.start (midiNoteToHz (48.f), 1.f, p);
    for (int i = 0; i < n; ++i)
    {
        float l = 0.f, r = 0.f;
        v.process (p, mx, 120.f, 0.f, l, r);
        dest[i] = 0.5f * (l + r);
        if (i == 64)
            v.release();
    }
}

// Peak of one tapped note. Level sits after distortion, so it scales the peak.
inline float phrasePeak (const Params& p, double sampleRate = 44100.0)
{
    Voice v;
    v.prepare (sampleRate);
    Matrix mx;
    v.start (midiNoteToHz (48.f), 1.f, p);
    float peak = 0.f;
    const int n = (int) (sampleRate * 3.0);
    for (int i = 0; i < n; ++i)
    {
        float l = 0.f, r = 0.f;
        v.process (p, mx, 120.f, 0.f, l, r);
        const float a = std::abs (0.5f * (l + r));
        if (a > peak)
            peak = a;
        if (i == 64)
            v.release();
        if (i > 64 && ! v.active())
            break;
    }
    return peak;
}

} // namespace f4rt
