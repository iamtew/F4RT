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

inline juce::Font boldFont (float pt)
{
    return juce::Font (juce::FontOptions (pt).withStyle ("Bold"));
}

} // namespace f4rt
