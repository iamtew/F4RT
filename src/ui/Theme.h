#pragma once

#include <JuceHeader.h>

namespace f4rt
{

inline const juce::Colour kGreen  { 0xff02f53a };
inline const juce::Colour kYellow { 0xffe4f20f };
inline const juce::Colour kLight  { 0xffe0e0e0 };
inline const juce::Colour kGrey   { 0xff666666 };
inline const juce::Colour kInk    { 0xff111111 };
inline const juce::Colour kChassis{ 0xff1a1a1a };

// Routing identity (not brand). Osc 1–4 / LFO 1–4 share a row; envs match Sound dests.
inline const juce::Colour kOscCol[4] {
    juce::Colour (0xff3dcc8c),
    juce::Colour (0xffff6a00),
    juce::Colour (0xff4c8dff),
    juce::Colour (0xffff4fa3)
};
inline const juce::Colour kEnvCol[6] {
    juce::Colour (0xffe4f20f), // Pitch
    juce::Colour (0xff02f53a), // Filter
    juce::Colour (0xffff6a00), // FM
    juce::Colour (0xff4c8dff), // Vowel
    juce::Colour (0xffff4fa3), // Grit
    juce::Colour (0xffe0e0e0)  // Loudness
};

inline juce::Font boldFont (float pt)
{
    return juce::Font (juce::FontOptions (pt).withStyle ("Bold"));
}

} // namespace f4rt
