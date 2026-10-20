#pragma once

#include "ParameterIDs.h"

#include <JuceHeader.h>

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto addF = [&] (const char* id, const String& name, NormalisableRange<float> r, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def));
    };
    auto addI = [&] (const char* id, const String& name, int def, const StringArray& choices)
    {
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, choices, def));
    };
    auto addB = [&] (const char* id, const String& name, bool def)
    {
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    const auto unit = NormalisableRange<float> (0.f, 1.f);
    const auto panR = NormalisableRange<float> (0.f, 1.f);
    const auto coarseR = NormalisableRange<float> (-24.f, 24.f, 1.f);
    const auto fineR = NormalisableRange<float> (-100.f, 100.f, 1.f);
    const auto hzR = NormalisableRange<float> (20.f, 12000.f, 0.1f, 0.3f);
    const auto tR = NormalisableRange<float> (0.0005f, 2.f, 0.0001f, 0.4f);
    const auto rateR = NormalisableRange<float> (0.05f, 30.f, 0.01f, 0.4f);

    const StringArray oscTypes { "Sine", "Tri", "Saw", "Square", "WT", "Noise", "User WT" };
    const StringArray vowels { "A", "E", "I", "O", "U" };
    const StringArray filtTypes { "LP", "HP", "BP", "Notch", "Comb", "SVF" };
    const StringArray distTypes { "Soft", "Hard", "Tube", "Fold", "Crush", "Digital" };
    const StringArray lfoShapes { "Sine", "Tri", "Ramp", "Random", "S&H", "User" };
    const StringArray polyModes { "Mono", "Legato", "Poly" };
    const StringArray randModes { "Clean", "Psytrance", "Forest", "Darkpsy", "Hi-Tech", "Experimental" };
    const StringArray fmRecipes { "Simple FM", "Metallic", "Alien", "Growl" };
    const StringArray srcNames {
        "Env Pitch", "Env Filt", "Env FM", "Env Form", "Env Dist", "Env Amp",
        "LFO1", "LFO2", "LFO3", "LFO4", "Vel", "AT",
        "Size", "Wet", "Anger", "Alien", "Squelch", "Rip", "Tox", "Random", "-"
    };
    const StringArray dstNames { "Off", "Pitch", "Cutoff", "Res", "FM", "Formant", "Dist", "Amp", "Pan" };
    const StringArray fxNames { "EQ", "Comp", "Delay", "Reverb", "Chorus", "Phaser", "Flanger", "Shift", "Grain" };

    for (int i = 0; i < 4; ++i)
    {
        const String n = String (i + 1);
        addF (ParamID::oVol[i], "Osc" + n + " Vol", unit, i == 0 ? 0.85f : 0.f);
        addF (ParamID::oPan[i], "Osc" + n + " Pan", panR, 0.5f);
        addF (ParamID::oCoarse[i], "Osc" + n + " Coarse", coarseR, 0.f);
        addF (ParamID::oFine[i], "Osc" + n + " Fine", fineR, 0.f);
        addF (ParamID::oPhase[i], "Osc" + n + " Phase", unit, 0.f);
        addF (ParamID::oWidth[i], "Osc" + n + " Width", unit, 0.f);
        addI (ParamID::oType[i], "Osc" + n + " Type", 0, oscTypes);
        for (int j = 0; j < 4; ++j)
            addF (ParamID::fm[i][j], "FM " + String (i + 1) + ">" + String (j + 1), unit, 0.f);
    }
    addF (ParamID::fmEnvDepth, "FM Env", unit, 0.f);
    addI (ParamID::fmRecipe, "FM Recipe", 0, fmRecipes);

    addI (ParamID::formA, "Form A", 0, vowels);
    addI (ParamID::formB, "Form B", 1, vowels);
    addF (ParamID::formMorph, "Morph", unit, 0.f);
    addF (ParamID::formTrack, "Form Track", unit, 0.f);
    addF (ParamID::formMix, "Form Mix", unit, 0.f);
    addI (ParamID::vowel, "Vowel", 0, vowels);

    addI (ParamID::filtType, "Filter", 0, filtTypes);
    addF (ParamID::cutoff, "Cutoff", hzR, 1800.f);
    addF (ParamID::res, "Res", unit, 0.25f);
    addF (ParamID::filtDrive, "F Drive", unit, 0.1f);
    addF (ParamID::keyTrack, "Key Track", unit, 0.3f);
    addF (ParamID::filtEnv, "F Env", unit, 0.4f);

    addI (ParamID::distType, "Dist", 0, distTypes);
    addF (ParamID::distDrive, "D Drive", unit, 0.15f);
    addF (ParamID::distTone, "D Tone", unit, 0.6f);
    addF (ParamID::distMix, "D Mix", unit, 0.2f);

    static const char* envN[] = { "Pitch", "Filt", "FM", "Form", "Dist", "Amp" };
    for (int i = 0; i < 6; ++i)
    {
        addF (ParamID::eAtk[i], String (envN[i]) + " A", tR, i == 5 ? 0.001f : 0.005f);
        addF (ParamID::eDec[i], String (envN[i]) + " D", tR, i == 5 ? 0.16f : 0.12f);
        addF (ParamID::eSus[i], String (envN[i]) + " S", unit, 0.f);
        addF (ParamID::eRel[i], String (envN[i]) + " R", tR, i == 5 ? 0.22f : 0.18f);
    }
    for (int i = 0; i < 4; ++i)
    {
        addI (ParamID::lShape[i], "LFO" + String (i + 1) + " Sh", 0, lfoShapes);
        addF (ParamID::lRate[i], "LFO" + String (i + 1) + " Rate", rateR, 2.f);
        addB (ParamID::lSync[i], "LFO" + String (i + 1) + " Sync", false);
        addF (ParamID::lAmt[i], "LFO" + String (i + 1) + " Amt", unit, 0.f);
    }

    static const char* macN[] = { "Size", "Wetness", "Anger", "Alienness", "Squelch", "Rip Length", "Toxicity" };
    static const float macD[] = { 0.45f, 0.35f, 0.2f, 0.1f, 0.3f, 0.4f, 0.05f };
    for (int i = 0; i < 7; ++i)
        addF (ParamID::macro[i], macN[i], unit, macD[i]);

    addI (ParamID::polyMode, "Poly", 2, polyModes);
    addF (ParamID::voices, "Voices", NormalisableRange<float> (1.f, 64.f, 1.f), 8.f);
    addF (ParamID::level, "Level", unit, 0.7f);
    addI (ParamID::randMode, "Rand Mode", 1, randModes);

    for (int i = 0; i < 32; ++i)
    {
        addI (ParamID::mxSrc[i], "Mx" + String (i) + " Src", 20, srcNames);
        addI (ParamID::mxDst[i], "Mx" + String (i) + " Dst", 0, dstNames);
        addF (ParamID::mxAmt[i], "Mx" + String (i) + " Amt", NormalisableRange<float> (-1.f, 1.f), 0.f);
        addB (ParamID::mxOn[i], "Mx" + String (i) + " On", false);
    }

    addB (ParamID::eqOn, "EQ", false);
    addF (ParamID::eqLo, "EQ Lo", NormalisableRange<float> (-12.f, 12.f), 0.f);
    addF (ParamID::eqMid, "EQ Mid", NormalisableRange<float> (-12.f, 12.f), 0.f);
    addF (ParamID::eqHi, "EQ Hi", NormalisableRange<float> (-12.f, 12.f), 0.f);
    addB (ParamID::cmpOn, "Comp", false);
    addF (ParamID::cmpTh, "Comp Th", NormalisableRange<float> (-40.f, 0.f), -18.f);
    addF (ParamID::cmpRt, "Comp Ratio", NormalisableRange<float> (1.f, 12.f), 3.f);
    addB (ParamID::dlyOn, "Delay", false);
    addF (ParamID::dlyT, "Delay T", NormalisableRange<float> (0.01f, 1.f, 0.001f, 0.4f), 0.25f);
    addF (ParamID::dlyFb, "Delay FB", unit, 0.35f);
    addB (ParamID::revOn, "Reverb", false);
    addF (ParamID::revSz, "Rev Size", unit, 0.4f);
    addF (ParamID::revDm, "Rev Damp", unit, 0.4f);
    addB (ParamID::choOn, "Chorus", false);
    addF (ParamID::choRt, "Cho Rate", rateR, 0.8f);
    addF (ParamID::choDp, "Cho Depth", unit, 0.3f);
    addB (ParamID::phOn, "Phaser", false);
    addF (ParamID::phRt, "Ph Rate", rateR, 0.4f);
    addF (ParamID::phDp, "Ph Depth", unit, 0.4f);
    addB (ParamID::flOn, "Flanger", false);
    addF (ParamID::flRt, "Fl Rate", rateR, 0.3f);
    addF (ParamID::flFb, "Fl FB", unit, 0.3f);
    addB (ParamID::shOn, "Shift", false);
    addF (ParamID::shHz, "Shift Hz", NormalisableRange<float> (-200.f, 200.f), 0.f);
    addB (ParamID::grOn, "Grain", false);
    addF (ParamID::grSz, "Grain Sz", NormalisableRange<float> (0.01f, 0.2f, 0.001f, 0.5f), 0.04f);
    addF (ParamID::grDn, "Grain Dn", unit, 0.4f);

    for (int i = 0; i < 9; ++i)
        addI (ParamID::fxSlot[i], "FX" + String (i + 1), i, fxNames);

    return layout;
}
