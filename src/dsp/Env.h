#pragma once

namespace f4rt
{

struct Adsr
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };

    Stage stage = Idle;
    float level = 0.f;
    float aInc = 1.f, dInc = 1.f, rInc = 1.f;
    float sustain = 0.f;

    void setTimes (float sr, float a, float d, float s, float r)
    {
        const float samp = sr > 1.f ? sr : 44100.f;
        aInc = 1.f / (samp * (a < 0.0005f ? 0.0005f : a));
        dInc = 1.f / (samp * (d < 0.001f ? 0.001f : d));
        rInc = 1.f / (samp * (r < 0.001f ? 0.001f : r));
        sustain = s;
    }

    void noteOn()
    {
        stage = Attack;
    }

    void noteOff()
    {
        if (stage != Idle)
            stage = Release;
    }

    void reset()
    {
        stage = Idle;
        level = 0.f;
    }

    float process()
    {
        switch (stage)
        {
            case Attack:
                level += aInc;
                if (level >= 1.f)
                {
                    level = 1.f;
                    stage = Decay;
                }
                break;
            case Decay:
                level -= dInc;
                if (level <= sustain)
                {
                    level = sustain;
                    stage = Sustain;
                }
                break;
            case Sustain:
                level = sustain;
                break;
            case Release:
                level -= rInc;
                if (level <= 0.f)
                {
                    level = 0.f;
                    stage = Idle;
                }
                break;
            default:
                level = 0.f;
                break;
        }
        return level;
    }

    bool active() const { return stage != Idle; }
};

} // namespace f4rt
