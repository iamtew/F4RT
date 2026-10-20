#pragma once

#include "ModMatrix.h"
#include "Voice.h"

#include <JuceHeader.h>

namespace f4rt
{

struct VoiceSlot
{
    Voice voice;
    int note = -1;
    int channel = 0;
    bool on = false;
};

struct SynthEngine
{
    static constexpr int kMax = 64;
    VoiceSlot slot[kMax];
    float userWt[kWtN] {};
    bool hasUserWt = false;
    float bend[16] {};
    float pressure[16] {};
    float modwheel = 0.f;
    int lastNote = -1;
    uint32_t stealAge = 0;

    void prepare (double sr)
    {
        makeBuiltinWt (userWt);
        for (auto& s : slot)
        {
            s.voice.prepare (sr);
            s.voice.userWt = userWt;
            s.note = -1;
            s.on = false;
        }
        for (float& b : bend)
            b = 0.f;
        for (float& p : pressure)
            p = 0.f;
    }

    int voiceCount (const Params& p) const
    {
        int n = p.voices;
        if (n < 1)
            n = 1;
        if (n > kMax)
            n = kMax;
        if (p.polyMode != 2)
            n = 1;
        return n;
    }

    int alloc (int nVoices, int, int)
    {
        for (int i = 0; i < nVoices; ++i)
            if (! slot[i].on && ! slot[i].voice.active())
                return i;
        int best = 0;
        float oldest = -1.f;
        for (int i = 0; i < nVoices; ++i)
            if (slot[i].voice.age > oldest)
            {
                oldest = slot[i].voice.age;
                best = i;
            }
        return best;
    }

    void noteOn (int note, float vel, int ch, const Params& p)
    {
        const int nV = voiceCount (p);
        if (p.polyMode == 1 && lastNote >= 0 && slot[0].on)
        {
            // legato: retune, keep envelopes
            slot[0].note = note;
            slot[0].channel = ch;
            slot[0].voice.noteHz = midiNoteToHz ((float) note);
            slot[0].voice.vel = vel;
            lastNote = note;
            return;
        }
        const int i = p.polyMode == 2 ? alloc (nV, note, ch) : 0;
        slot[i].note = note;
        slot[i].channel = ch;
        slot[i].on = true;
        slot[i].voice.start (midiNoteToHz ((float) note), vel, p);
        lastNote = note;
    }

    void noteOff (int note, int ch)
    {
        for (auto& s : slot)
            if (s.on && s.note == note && (ch < 0 || s.channel == ch))
            {
                s.on = false;
                s.voice.release();
            }
        if (lastNote == note)
            lastNote = -1;
    }

    void handleMidi (const juce::MidiMessage& m, const Params& p)
    {
        const int ch = m.getChannel() - 1;
        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity(), ch, p);
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber(), ch);
        else if (m.isPitchWheel() && ch >= 0 && ch < 16)
            bend[ch] = ((float) m.getPitchWheelValue() - 8192.f) / 8192.f * 2.f; // ±2 st, MPE hosts scale
        else if (m.isChannelPressure() && ch >= 0 && ch < 16)
            pressure[ch] = m.getChannelPressureValue() / 127.f;
        else if (m.isController() && m.getControllerNumber() == 1)
            modwheel = m.getControllerValue() / 127.f;
        else if (m.isAllNotesOff() || m.isAllSoundOff())
        {
            for (auto& s : slot)
            {
                s.on = false;
                s.voice.reset();
            }
            lastNote = -1;
        }
    }

    void render (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi,
                 const Params& p, const Matrix& mx, float bpm)
    {
        const int n = buffer.getNumSamples();
        const int nV = voiceCount (p);
        int sample = 0;
        for (const auto meta : midi)
        {
            const int ts = juce::jlimit (0, n, meta.samplePosition);
            renderSlice (buffer, sample, ts - sample, p, mx, bpm, nV);
            sample = ts;
            handleMidi (meta.getMessage(), p);
        }
        renderSlice (buffer, sample, n - sample, p, mx, bpm, nV);
        midi.clear();
    }

    void renderSlice (juce::AudioBuffer<float>& buffer, int start, int count,
                      const Params& p, const Matrix& mx, float bpm, int nV)
    {
        if (count <= 0)
            return;
        auto* l = buffer.getWritePointer (0);
        float* r = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
        for (int i = 0; i < count; ++i)
        {
            float sl = 0.f, sr = 0.f;
            for (int v = 0; v < nV; ++v)
            {
                auto& s = slot[v];
                if (! s.voice.active() && ! s.on)
                    continue;
                const int ch = s.channel < 0 ? 0 : (s.channel > 15 ? 15 : s.channel);
                s.voice.bend = bend[ch] * 12.f * 0.1667f * 12.f; // keep small
                s.voice.bend = bend[ch] * 2.f;
                s.voice.aftertouch = pressure[ch];
                float vl = 0.f, vr = 0.f;
                s.voice.process (p, mx, bpm, modwheel, vl, vr);
                sl += vl;
                sr += vr;
            }
            const int idx = start + i;
            l[idx] += sl;
            if (r != nullptr)
                r[idx] += sr;
            else
                l[idx] += sr;
        }
    }

    void loadUserWt (const float* src, int n)
    {
        if (src == nullptr || n <= 0)
            return;
        for (int i = 0; i < kWtN; ++i)
        {
            const float x = (float) i * (float) n / (float) kWtN;
            int a = (int) x;
            if (a >= n)
                a = n - 1;
            int b = a + 1;
            if (b >= n)
                b = n - 1;
            const float f = x - (float) a;
            userWt[i] = src[a] + (src[b] - src[a]) * f;
        }
        hasUserWt = true;
        for (auto& s : slot)
            s.voice.userWt = userWt;
    }
};

} // namespace f4rt
