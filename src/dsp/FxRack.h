#pragma once

#include "../parameters/ParameterIDs.h"

#include <JuceHeader.h>
#include <cmath>

namespace f4rt
{

inline float rawP (juce::AudioProcessorValueTreeState& s, const char* id)
{
    if (auto* p = s.getRawParameterValue (id))
        return p->load();
    return 0.f;
}

struct FxRack
{
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delay { 192000 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> grain { 192000 };
    juce::dsp::Reverb reverb;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;
    juce::dsp::Compressor<float> comp;
    juce::dsp::IIR::Filter<float> eqLo, eqHi;
    float flState = 0.f, flStateR = 0.f, flLfo = 0.f;
    float shPhase = 0.f;
    float grPos = 0.f;
    double sr = 44100.0;
    int maxBlock = 512;

    void prepare (double sampleRate, int samplesPerBlock, int ch)
    {
        sr = sampleRate > 1.0 ? sampleRate : 44100.0;
        maxBlock = samplesPerBlock;
        juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, (juce::uint32) juce::jmax (1, ch) };
        delay.setMaximumDelayInSamples ((int) (sr * 2.0));
        delay.prepare (spec);
        grain.setMaximumDelayInSamples ((int) (sr * 2.0));
        grain.prepare (spec);
        reverb.prepare (spec);
        chorus.prepare (spec);
        phaser.prepare (spec);
        comp.prepare (spec);
        eqLo.prepare (spec);
        eqHi.prepare (spec);
        reset();
    }

    void reset()
    {
        delay.reset();
        grain.reset();
        reverb.reset();
        chorus.reset();
        phaser.reset();
        comp.reset();
        eqLo.reset();
        eqHi.reset();
        flState = flStateR = flLfo = shPhase = grPos = 0.f;
    }

    void processOne (int kind, juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        switch (kind)
        {
            case 0:
                if (rawP (apvts, ParamID::eqOn) > 0.5f)
                {
                    eqLo.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowShelf (
                        sr, 120.f, 0.7f, juce::Decibels::decibelsToGain (rawP (apvts, ParamID::eqLo)));
                    eqHi.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                        sr, 4000.f, 0.7f, juce::Decibels::decibelsToGain (rawP (apvts, ParamID::eqHi)));
                    eqLo.process (ctx);
                    eqHi.process (ctx);
                    const float mid = juce::Decibels::decibelsToGain (rawP (apvts, ParamID::eqMid));
                    buffer.applyGain (mid);
                }
                break;
            case 1:
                if (rawP (apvts, ParamID::cmpOn) > 0.5f)
                {
                    comp.setThreshold (rawP (apvts, ParamID::cmpTh));
                    comp.setRatio (rawP (apvts, ParamID::cmpRt));
                    comp.setAttack (10.f);
                    comp.setRelease (80.f);
                    comp.process (ctx);
                }
                break;
            case 2:
                if (rawP (apvts, ParamID::dlyOn) > 0.5f)
                {
                    const float t = rawP (apvts, ParamID::dlyT);
                    const float fb = rawP (apvts, ParamID::dlyFb);
                    delay.setDelay (t * (float) sr);
                    // One delay line per channel. A panned note stays on that side.
                    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                    {
                        auto* s = buffer.getWritePointer (ch);
                        for (int i = 0; i < buffer.getNumSamples(); ++i)
                        {
                            const float in = s[i];
                            const float d = delay.popSample (ch);
                            delay.pushSample (ch, in + d * fb);
                            s[i] = in + d * 0.5f;
                        }
                    }
                }
                break;
            case 3:
                if (rawP (apvts, ParamID::revOn) > 0.5f)
                {
                    juce::dsp::Reverb::Parameters rp;
                    rp.roomSize = rawP (apvts, ParamID::revSz);
                    rp.damping = rawP (apvts, ParamID::revDm);
                    rp.wetLevel = 0.35f;
                    rp.dryLevel = 0.8f;
                    reverb.setParameters (rp);
                    reverb.process (ctx);
                }
                break;
            case 4:
                if (rawP (apvts, ParamID::choOn) > 0.5f)
                {
                    chorus.setRate (rawP (apvts, ParamID::choRt));
                    chorus.setDepth (rawP (apvts, ParamID::choDp));
                    chorus.setMix (0.4f);
                    chorus.process (ctx);
                }
                break;
            case 5:
                if (rawP (apvts, ParamID::phOn) > 0.5f)
                {
                    phaser.setRate (rawP (apvts, ParamID::phRt));
                    phaser.setDepth (rawP (apvts, ParamID::phDp));
                    phaser.setMix (0.4f);
                    phaser.process (ctx);
                }
                break;
            case 6:
                if (rawP (apvts, ParamID::flOn) > 0.5f)
                {
                    const float rate = rawP (apvts, ParamID::flRt);
                    const float fb = rawP (apvts, ParamID::flFb);
                    const int nCh = buffer.getNumChannels();
                    float* s0 = buffer.getWritePointer (0);
                    float* s1 = nCh > 1 ? buffer.getWritePointer (1) : nullptr;
                    float* st0 = &flState;
                    float* st1 = &flStateR;
                    for (int i = 0; i < buffer.getNumSamples(); ++i)
                    {
                        flLfo += rate / (float) sr;
                        if (flLfo > 1.f)
                            flLfo -= 1.f;
                        const float d = 0.002f + 0.002f * std::sin (flLfo * 6.283185f);
                        const float depth = 0.4f * (0.5f + d * 100.f);
                        const float mix0 = s0[i] + *st0 * fb;
                        *st0 = mix0;
                        s0[i] = s0[i] + mix0 * depth;
                        if (s1 != nullptr)
                        {
                            const float mix1 = s1[i] + *st1 * fb;
                            *st1 = mix1;
                            s1[i] = s1[i] + mix1 * depth;
                        }
                    }
                }
                break;
            case 7:
                if (rawP (apvts, ParamID::shOn) > 0.5f)
                {
                    const float hz = rawP (apvts, ParamID::shHz);
                    auto* l = buffer.getWritePointer (0);
                    float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
                    for (int i = 0; i < buffer.getNumSamples(); ++i)
                    {
                        shPhase += hz / (float) sr;
                        if (shPhase > 1.f)
                            shPhase -= 1.f;
                        if (shPhase < 0.f)
                            shPhase += 1.f;
                        const float m = std::cos (shPhase * 6.283185f);
                        l[i] *= m;
                        if (r != nullptr)
                            r[i] *= m;
                    }
                }
                break;
            default:
                if (rawP (apvts, ParamID::grOn) > 0.5f)
                {
                    const float sz = rawP (apvts, ParamID::grSz);
                    const float dn = rawP (apvts, ParamID::grDn);
                    const int gN = juce::jmax (32, (int) (sz * (float) sr));
                    const int nCh = buffer.getNumChannels();
                    for (int i = 0; i < buffer.getNumSamples(); ++i)
                    {
                        grPos += dn * 0.35f;
                        if (grPos > 1.f)
                            grPos -= 1.f;
                        const float env = std::sin (grPos * 3.141592f);
                        const float delaySamps = (float) gN * grPos;
                        for (int ch = 0; ch < nCh; ++ch)
                        {
                            auto* s = buffer.getWritePointer (ch);
                            grain.pushSample (ch, s[i]);
                            const float g = grain.popSample (ch, delaySamps);
                            s[i] = s[i] * (1.f - dn * 0.5f) + g * env * dn;
                        }
                    }
                }
                break;
        }
    }

    void process (juce::AudioBuffer<float>& buffer, juce::AudioProcessorValueTreeState& apvts)
    {
        int order[9];
        bool used[9] {};
        for (int i = 0; i < 9; ++i)
        {
            int k = juce::roundToInt (rawP (apvts, ParamID::fxSlot[i]));
            k = juce::jlimit (0, 8, k);
            if (used[k])
            {
                for (int j = 0; j < 9; ++j)
                    if (! used[j])
                    {
                        k = j;
                        break;
                    }
            }
            used[k] = true;
            order[i] = k;
        }
        for (int i = 0; i < 9; ++i)
            processOne (order[i], buffer, apvts);
    }
};

} // namespace f4rt
