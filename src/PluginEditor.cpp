#include "PluginEditor.h"
#include "parameters/FactoryBank.h"
#include "ui/VersionCompare.h"

#include <cmath>
#include <thread>

namespace
{
constexpr int kEditorW = 1120;
constexpr int kEditorH = 720;
constexpr const char* kPageNames[] = { "Sound", "Motion", "Matrix", "FX", "Patches" };
constexpr int kPad = 8;
constexpr int kTitleH = 40;
constexpr int kTitleHitW = 150;
constexpr int kTabH = 28;
constexpr int kLabelH = 12;
constexpr int kHintH = 10;
constexpr int kSectionHead = 20;
constexpr int kCatSize = 20;  // factory presets per category (FactoryBank: index / 20)
constexpr int kCatCount = f4rt::kFactoryCount / kCatSize;

// Patch menu ids. Factory and user entries are offsets so the ranges cannot collide.
constexpr int kMenuInit = 1;
constexpr int kMenuSave = 2;
constexpr int kMenuSaveAs = 3;
constexpr int kMenuExport = 4;
constexpr int kMenuImport = 5;
constexpr int kMenuFactory = 100;   // + factory index (0..199)
constexpr int kMenuUser = 1000;     // + index into patchNames()

using f4rt::kGreen;
using f4rt::kYellow;
using f4rt::kLight;
using f4rt::kGrey;
using f4rt::kInk;
using f4rt::kChassis;
using f4rt::boldFont;

juce::String factoryLabel (int i)
{
    char name[64];
    f4rt::factoryName (i, name, 64);
    return name;
}
} // namespace

// ---------------------------------------------------------------- Cells

F4RTAudioProcessorEditor::Knob::Knob (juce::AudioProcessorValueTreeState& st,
                                      const juce::String& id, const juce::String& name,
                                      const juce::String& tip, const juce::String& hintText)
{
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, kLight);
    addAndMakeVisible (label);
    label.setVisible (name.isNotEmpty());
    hint.setText (hintText, juce::dontSendNotification);
    hint.setJustificationType (juce::Justification::centred);
    hint.setColour (juce::Label::textColourId, kGrey);
    hint.setFont (boldFont (9.f));
    addAndMakeVisible (hint);
    hint.setVisible (hintText.isNotEmpty());
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 12);
    slider.setColour (juce::Slider::textBoxTextColourId, kLight);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);
    att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, id, slider);
    if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (st.getParameter (id)))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    setTooltip (tip);
    slider.setTooltip (tip);
}

void F4RTAudioProcessorEditor::Knob::resized()
{
    auto r = getLocalBounds();
    if (label.getText().isNotEmpty())
        label.setBounds (r.removeFromTop (kLabelH));
    if (hint.isVisible())
        hint.setBounds (r.removeFromTop (kHintH));
    slider.setBounds (r);
}

F4RTAudioProcessorEditor::Combo::Combo (juce::AudioProcessorValueTreeState& st,
                                        const juce::String& id, const juce::String& name,
                                        const juce::String& tip)
{
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, kLight);
    addAndMakeVisible (label);
    label.setVisible (name.isNotEmpty());
    // ComboBoxAttachment does not add items; without this every box is empty.
    if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (st.getParameter (id)))
        box.addItemList (c->choices, 1);
    addAndMakeVisible (box);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, id, box);
    setTooltip (tip);
    box.setTooltip (tip);
}

void F4RTAudioProcessorEditor::Combo::resized()
{
    auto r = getLocalBounds();
    if (label.getText().isNotEmpty())
        label.setBounds (r.removeFromTop (kLabelH));
    box.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 4, 160), 24));
}

F4RTAudioProcessorEditor::Toggle::Toggle (juce::AudioProcessorValueTreeState& st,
                                          const juce::String& id, const juce::String& name,
                                          const juce::String& tip)
{
    btn.setButtonText (name);
    addAndMakeVisible (btn);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (st, id, btn);
    setTooltip (tip);
    btn.setTooltip (tip);
}

void F4RTAudioProcessorEditor::Toggle::resized()
{
    btn.setBounds (getLocalBounds().withSizeKeepingCentre (juce::jmin (getWidth() - 4, 96), 24));
}

// ---------------------------------------------------------------- Look

void F4RTAudioProcessorEditor::PanelLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                           float pos, float startAngle, float endAngle,
                                                           juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (1.f);
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
    if (d < 8.f)
        return;

    auto area = bounds.withSizeKeepingCentre (d, d);
    const auto c = area.getCentre();
    const float tickW = juce::jmax (1.f, d * 0.018f);

    g.setColour (kLight.withAlpha (0.75f));
    constexpr int nTicks = 11;
    for (int i = 0; i < nTicks; ++i)
    {
        const float t = (float) i / (float) (nTicks - 1);
        const float a = startAngle + t * (endAngle - startAngle);
        const auto p0 = c.getPointOnCircumference (d * 0.48f, a);
        const auto p1 = c.getPointOnCircumference (d * 0.40f, a);
        g.drawLine (p0.x, p0.y, p1.x, p1.y, tickW);
    }

    const float bodyD = d * 0.72f;
    auto body = area.withSizeKeepingCentre (bodyD, bodyD);
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillEllipse (body.translated (0.f, d * 0.04f).expanded (d * 0.01f));
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (body.translated (0.f, d * 0.018f));

    g.setGradientFill (juce::ColourGradient (kChassis.brighter (0.18f), c.x, body.getY(),
                                              kInk, c.x, body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (kLight.withAlpha (0.3f));
    g.drawEllipse (body.reduced (0.6f), juce::jmax (1.f, d * 0.012f));

    const float capD = bodyD * 0.5f;
    auto cap = area.withSizeKeepingCentre (capD, capD);
    g.setColour (kGreen);
    g.fillEllipse (cap);

    const float angle = startAngle + pos * (endAngle - startAngle);
    juce::Path pointer;
    pointer.startNewSubPath (c.getPointOnCircumference (bodyD * 0.32f, angle));
    pointer.lineTo (c.getPointOnCircumference (bodyD * 0.46f, angle));
    g.setColour (kLight);
    g.strokePath (pointer, juce::PathStrokeType (juce::jmax (1.15f, d * 0.028f),
                                                  juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    const float pip = juce::jmax (1.6f, d * 0.045f);
    const auto pipC = c.getPointOnCircumference (capD * 0.28f, angle);
    g.fillEllipse (pipC.x - pip * 0.5f, pipC.y - pip * 0.5f, pip, pip);
}

void F4RTAudioProcessorEditor::PanelLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (2.f);
    g.setColour (b.getToggleState() ? kGreen.withAlpha (0.85f) : (over ? kInk.brighter (0.12f) : kInk));
    g.fillRoundedRectangle (bounds, 3.f);
    g.setColour (kGrey);
    g.drawRoundedRectangle (bounds.reduced (0.5f), 3.f, 1.f);
    g.setFont (boldFont (11.f));
    g.setColour (b.getToggleState() ? kInk : kLight);
    g.drawText (b.getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
}

void F4RTAudioProcessorEditor::PanelLnF::drawComboBox (juce::Graphics& g, int width, int height, bool,
                                                       int, int, int, int, juce::ComboBox&)
{
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) width, (float) height);
    g.setColour (kInk);
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (kYellow.withAlpha (0.5f));
    g.drawRoundedRectangle (r.reduced (0.5f), 3.f, 1.f);
    // Small down-chevron so it reads as a dropdown.
    juce::Path p;
    const float cx = (float) width - 9.f, cy = (float) height * 0.5f;
    p.addTriangle (cx - 4.f, cy - 2.f, cx + 4.f, cy - 2.f, cx, cy + 3.f);
    g.setColour (kYellow);
    g.fillPath (p);
}

juce::Font F4RTAudioProcessorEditor::PanelLnF::getLabelFont (juce::Label&) { return boldFont (11.f); }
juce::Font F4RTAudioProcessorEditor::PanelLnF::getComboBoxFont (juce::ComboBox&) { return boldFont (11.f); }

void F4RTAudioProcessorEditor::PanelLnF::drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height)
{
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) width, (float) height);
    g.setColour (kInk);
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (kGreen);
    g.drawRoundedRectangle (r.reduced (0.5f), 3.f, 1.f);
    g.setColour (kLight);
    g.setFont (boldFont (12.f));
    g.drawFittedText (text, r.reduced (6.f, 4.f).toNearestInt(), juce::Justification::centredLeft, 4);
}

// ---------------------------------------------------------------- About

F4RTAudioProcessorEditor::AboutBox::AboutBox()
{
    status.setJustificationType (juce::Justification::centred);
    status.setColour (juce::Label::textColourId, kLight);
    addAndMakeVisible (status);
    // ToggleButton is only used for the LnF look here, so it must not latch.
    checkBtn.setClickingTogglesState (false);
    closeBtn.setClickingTogglesState (false);
    checkBtn.onClick = [this] { checkForUpdate(); };
    closeBtn.onClick = [this] { setVisible (false); };
    addAndMakeVisible (checkBtn);
    addAndMakeVisible (closeBtn);
    releasesBtn.setColour (juce::HyperlinkButton::textColourId, kYellow);
    releasesBtn.setVisible (false);
    addAndMakeVisible (releasesBtn);
}

void F4RTAudioProcessorEditor::AboutBox::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.55f));
    auto card = cardBounds().toFloat();
    g.setColour (kInk);
    g.fillRoundedRectangle (card, 6.f);
    g.setColour (kGreen);
    g.drawRoundedRectangle (card.reduced (0.5f), 6.f, 1.f);
    auto r = cardBounds().reduced (16, 14);
    g.setColour (kYellow);
    g.setFont (boldFont (20.f));
    g.drawText ("F4RT", r.removeFromTop (26), juce::Justification::centred, false);
    g.setFont (boldFont (12.f));
    g.setColour (kLight);
    g.drawText ("v" JucePlugin_VersionString, r.removeFromTop (18), juce::Justification::centred, false);
    g.drawText ("Stupid Systems LLC", r.removeFromTop (16), juce::Justification::centred, false);
    g.drawText ("MIT", r.removeFromTop (16), juce::Justification::centred, false);
}

void F4RTAudioProcessorEditor::AboutBox::resized()
{
    auto r = cardBounds().reduced (16, 14);
    r.removeFromTop (26 + 18 + 16 + 16 + 8);
    checkBtn.setBounds (r.removeFromTop (22).withSizeKeepingCentre (160, 22));
    r.removeFromTop (6);
    status.setBounds (r.removeFromTop (22));
    r.removeFromTop (4);
    releasesBtn.setBounds (r.removeFromTop (20).withSizeKeepingCentre (140, 20));
    closeBtn.setBounds (cardBounds().removeFromBottom (36).reduced (90, 8));
}

void F4RTAudioProcessorEditor::AboutBox::mouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && ! cardBounds().contains (e.getPosition()))
        setVisible (false);
}

juce::Rectangle<int> F4RTAudioProcessorEditor::AboutBox::cardBounds() const
{
    return getLocalBounds().withSizeKeepingCentre (320, 240);
}

void F4RTAudioProcessorEditor::AboutBox::applyResult (const juce::String& text, bool hasUpdate)
{
    fetching.store (false);
    status.setText (text, juce::dontSendNotification);
    releasesBtn.setVisible (hasUpdate);
}

void F4RTAudioProcessorEditor::AboutBox::checkForUpdate()
{
    if (fetching.exchange (true))
        return;
    status.setText ("Checking...", juce::dontSendNotification);
    releasesBtn.setVisible (false);
    // Network on a detached thread; the result hops back to the message thread
    // and is dropped if the editor closed meanwhile (SafePointer goes null).
    juce::Component::SafePointer<AboutBox> safe (this);
    std::thread ([safe]
    {
        juce::String text;
        bool hasUpdate = false;
        juce::URL url ("https://api.github.com/repos/iamtew/F4RT/tags");
        const auto opts = juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                              .withExtraHeaders ("User-Agent: F4RT\nAccept: application/vnd.github+json")
                              .withConnectionTimeoutMs (8000);
        if (auto stream = url.createInputStream (opts))
        {
            const auto parsed = juce::JSON::parse (stream->readEntireStreamAsString());
            if (auto* arr = parsed.getArray())
            {
                std::vector<std::string> names;
                for (const auto& v : *arr)
                    if (auto* obj = v.getDynamicObject())
                        names.push_back (obj->getProperty ("name").toString().toStdString());
                const auto newest = f4rt::newestNewerTag (names, JucePlugin_VersionString);
                if (newest.empty())
                    text = "You are up to date.";
                else
                {
                    text = "Newer version available: " + juce::String (newest);
                    hasUpdate = true;
                }
            }
            else
                text = "Could not parse tags.";
        }
        else
            text = "Could not reach GitHub.";
        juce::MessageManager::callAsync ([safe, text, hasUpdate]
        {
            if (safe != nullptr)
                safe->applyResult (text, hasUpdate);
        });
    }).detach();
}

// ---------------------------------------------------------------- Top bar widgets

void F4RTAudioProcessorEditor::PatchBar::setText (const juce::String& t)
{
    if (text == t)
        return;
    text = t;
    repaint();
}

void F4RTAudioProcessorEditor::PatchBar::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (kInk);
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (kGrey);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.f, 1.f);

    auto left = r.removeFromLeft (18.f);
    auto right = r.removeFromRight (18.f);
    auto chev = [] (juce::Rectangle<float> box, bool back)
    {
        juce::Path p;
        const float cx = box.getCentreX();
        const float cy = box.getCentreY();
        if (back)
            p.addTriangle (cx + 3.f, cy - 5.f, cx - 4.f, cy, cx + 3.f, cy + 5.f);
        else
            p.addTriangle (cx - 3.f, cy - 5.f, cx + 4.f, cy, cx - 3.f, cy + 5.f);
        return p;
    };
    g.setColour (kGreen);
    g.fillPath (chev (left, true));
    g.fillPath (chev (right, false));
    g.setColour (kLight);
    g.setFont (boldFont (12.f));
    g.drawText (text, r.toNearestInt(), juce::Justification::centred, true);
}

void F4RTAudioProcessorEditor::PatchBar::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked())
        return;
    if (e.x < 20)
    {
        if (onStep)
            onStep (-1);
    }
    else if (e.x > getWidth() - 20)
    {
        if (onStep)
            onStep (1);
    }
    else if (onOpen)
        onOpen();
}

void F4RTAudioProcessorEditor::PageTabs::paint (juce::Graphics& g)
{
    const int n = F4RTAudioProcessorEditor::kPageCount;
    const float w = (float) getWidth() / (float) n;
    for (int i = 0; i < n; ++i)
    {
        auto t = juce::Rectangle<float> (i * w, 0.f, w, (float) getHeight()).reduced (2.f, 3.f);
        const bool on = i == ed.editorPage;
        g.setColour (on ? kInk.brighter (0.16f) : kInk);
        g.fillRoundedRectangle (t, 4.f);
        g.setColour (on ? kYellow : kGrey);
        g.drawRoundedRectangle (t.reduced (0.5f), 4.f, 1.f);
        g.setFont (boldFont (12.f));
        g.setColour (on ? kYellow : kLight);
        g.drawText (kPageNames[i], t.toNearestInt(), juce::Justification::centred, false);
    }
}

void F4RTAudioProcessorEditor::PageTabs::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked() || getWidth() <= 0)
        return;
    const int i = juce::jlimit (0, F4RTAudioProcessorEditor::kPageCount - 1,
                                e.x * F4RTAudioProcessorEditor::kPageCount / getWidth());
    ed.setEditorPage (i);
}

F4RTAudioProcessorEditor::VolumeStrip::VolumeStrip (F4RTAudioProcessorEditor& ed)
    : editor (ed)
{
    slider.setLookAndFeel (&lnf);
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    slider.setOpaque (false);
    slider.setColour (juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::trackColourId, juce::Colours::transparentBlack);
    const juce::String levelTip = "Output level. Right side is the spectrum. Yellow = clip.";
    setTooltip (levelTip);
    slider.setTooltip (levelTip);
    if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (editor.proc.apvts.getParameter (ParamID::level)))
        slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    sliderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        editor.proc.apvts, ParamID::level, slider);
    addAndMakeVisible (slider);
}

F4RTAudioProcessorEditor::VolumeStrip::~VolumeStrip()
{
    slider.setLookAndFeel (nullptr);
}

juce::Rectangle<float> F4RTAudioProcessorEditor::VolumeStrip::faderArea() const
{
    auto r = getLocalBounds().toFloat();
    return r.removeFromLeft (r.getWidth() * 5.f / 9.f);
}

juce::Rectangle<float> F4RTAudioProcessorEditor::VolumeStrip::scopeArea() const
{
    auto r = getLocalBounds().toFloat();
    r.removeFromLeft (r.getWidth() * 5.f / 9.f + 4.f);
    return r;
}

void F4RTAudioProcessorEditor::VolumeStrip::resized()
{
    slider.setBounds (faderArea().toNearestInt());
}

void F4RTAudioProcessorEditor::VolumeStrip::LnF::drawLinearSlider (juce::Graphics& g, int, int y, int, int h,
                                                                  float pos, float, float,
                                                                  juce::Slider::SliderStyle, juce::Slider&)
{
    const float by = (float) (y + h);
    juce::Path p;
    p.addTriangle (pos - 5.f, by, pos + 5.f, by, pos, by - 7.f);
    g.setColour (kInk);
    g.fillPath (p);
    g.setColour (kLight);
    g.strokePath (p, juce::PathStrokeType (1.f));
}

void F4RTAudioProcessorEditor::VolumeStrip::paint (juce::Graphics& g)
{
    auto fader = faderArea();
    auto scope = scopeArea();

    g.setColour (kInk);
    g.fillRoundedRectangle (fader, 3.f);
    g.fillRoundedRectangle (scope, 3.f);

    const float pos = (float) slider.valueToProportionOfLength (slider.getValue());
    auto fill = fader.withWidth (juce::jmax (2.f, fader.getWidth() * pos));
    g.setColour (clipped ? kYellow : kGrey);
    g.fillRoundedRectangle (fill, 3.f);

    g.setColour (kLight.withAlpha (0.35f));
    for (int i = 1; i < 8; ++i)
    {
        const float x = fader.getX() + fader.getWidth() * (float) i / 8.f;
        const float tickH = (i == 4) ? 6.f : 3.f;
        g.drawLine (x, fader.getBottom() - 2.f, x, fader.getBottom() - 2.f - tickH, 1.f);
    }

    // Output spectrum on a 20 Hz..20 kHz log axis, read from the analyser's last finished FFT.
    const auto& a = editor.proc.analyser;
    const float* mag = a.magDb[a.magBank];
    const int n = juce::jlimit (2, 256, (int) scope.getWidth());
    const double hzPerBin = a.sampleRate / (double) f4rt::Analyser::fftSize;
    juce::Path spec;
    spec.startNewSubPath (scope.getX(), scope.getBottom());
    for (int i = 0; i < n; ++i)
    {
        const float x01 = (float) i / (float) (n - 1);
        const double hz = 20.0 * std::pow (1000.0, (double) x01);
        const int bin = juce::jlimit (1, f4rt::Analyser::bins - 1, (int) std::lround (hz / hzPerBin));
        const float y01 = 1.f - juce::jlimit (0.f, 1.f, (mag[bin] + 84.f) / 72.f);
        spec.lineTo (scope.getX() + x01 * scope.getWidth(), scope.getY() + y01 * scope.getHeight());
    }
    spec.lineTo (scope.getRight(), scope.getBottom());
    spec.closeSubPath();
    g.setColour ((clipped ? kYellow : kGreen).withAlpha (0.7f));
    g.fillPath (spec);
}

void F4RTAudioProcessorEditor::VolumeStrip::refresh()
{
    if (editor.proc.clipFlag.exchange (false, std::memory_order_relaxed))
        clipUntil = juce::Time::getMillisecondCounter() + 800;
    clipped = juce::Time::getMillisecondCounter() < clipUntil;
    repaint();
}

// ---------------------------------------------------------------- Viz

void F4RTAudioProcessorEditor::Viz::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    g.setColour (kInk);
    g.fillRoundedRectangle (r, 4.f);
    auto& a = ed.proc.analyser;
    if (mode == 0)
    {
        float s[256];
        a.copyWave (s, 256);
        juce::Path p;
        for (int i = 0; i < 256; ++i)
        {
            const float x = r.getX() + r.getWidth() * i / 255.f;
            const float y = r.getCentreY() - s[i] * r.getHeight() * 0.4f;
            if (i == 0)
                p.startNewSubPath (x, y);
            else
                p.lineTo (x, y);
        }
        g.setColour (kGreen);
        g.strokePath (p, juce::PathStrokeType (1.2f));
    }
    else if (mode == 1)
    {
        const int bank = a.magBank;
        for (int i = 1; i < f4rt::Analyser::bins; ++i)
        {
            const float n = (a.magDb[bank][i] + 80.f) / 80.f;
            const float x = r.getX() + r.getWidth() * i / (float) f4rt::Analyser::bins;
            g.setColour (kYellow.withAlpha (juce::jlimit (0.f, 1.f, n)));
            g.drawLine (x, r.getBottom(), x, r.getBottom() - n * r.getHeight(), 1.f);
        }
    }
    else if (mode == 2)
    {
        const float cw = r.getWidth() / (float) f4rt::Analyser::specCols;
        const float rh = r.getHeight() / (float) f4rt::Analyser::specRows;
        for (int c = 0; c < f4rt::Analyser::specCols; ++c)
            for (int row = 0; row < f4rt::Analyser::specRows; ++row)
            {
                const float v = a.spec[c][row];
                g.setColour (kGreen.interpolatedWith (kYellow, v).withAlpha (v));
                g.fillRect (r.getX() + c * cw, r.getBottom() - (row + 1) * rh, cw, rh);
            }
    }
    else if (mode == 3)
    {
        const float cw = r.getWidth() / 4.f, rh = r.getHeight() / 4.f;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
            {
                g.setColour (kGreen.withAlpha (a.fmVis[i][j]));
                g.fillRect (r.getX() + j * cw + 2, r.getY() + i * rh + 2, cw - 4, rh - 4);
            }
    }
    else if (mode == 4)
    {
        for (int i = 0; i < 6; ++i)
        {
            auto b = r.removeFromLeft (r.getWidth() / (6 - i)).reduced (4);
            g.setColour (kGrey);
            g.drawRect (b);
            g.setColour (kYellow);
            g.fillRect (b.removeFromBottom (b.getHeight() * ed.proc.envMon[i]));
        }
    }
    else
    {
        // Active matrix routes, by name.
        g.setColour (kLight);
        g.setFont (boldFont (11.f));
        auto& st = ed.proc.apvts;
        int y = 8;
        for (int i = 0; i < f4rt::kMatrixSlots && y < (int) r.getHeight(); ++i)
        {
            if (st.getRawParameterValue (ParamID::mxOn[i])->load() < 0.5f)
                continue;
            g.drawText (juce::String (i + 1) + ": " + st.getParameter (ParamID::mxSrc[i])->getCurrentValueAsText()
                            + " -> " + st.getParameter (ParamID::mxDst[i])->getCurrentValueAsText()
                            + "  " + st.getParameter (ParamID::mxAmt[i])->getCurrentValueAsText(),
                        (int) r.getX() + 8, (int) r.getY() + y, (int) r.getWidth() - 16, 14,
                        juce::Justification::centredLeft, true);
            y += 16;
        }
        if (y == 8)
            g.drawText ("No active routes. Switch a row On on the Matrix tab.", r.toNearestInt(),
                        juce::Justification::centred, false);
    }
}

// ---------------------------------------------------------------- Browser

void F4RTAudioProcessorEditor::BrowserModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel)
{
    if (row < 0 || row >= rows.size())
        return;
    g.fillAll (sel ? kGreen.withAlpha (0.35f) : kInk);
    g.setColour (kLight);
    g.setFont (boldFont (12.f));
    g.drawText (rows[row], 6, 0, w - 12, h, juce::Justification::centredLeft, true);
}

void F4RTAudioProcessorEditor::BrowserModel::listBoxItemClicked (int row, const juce::MouseEvent&)
{
    if (ed == nullptr || row < 0 || row >= rows.size())
        return;
    auto name = rows[row];
    if (name.endsWith (" *"))  // favourite marker
        name = name.dropLastCharacters (2);
    // Factory rows are "007 | Name  [tags]"; user rows are the bare file name.
    if (name.contains (" | "))
        ed->proc.loadFactory (name.upToFirstOccurrenceOf (" |", false, false).getIntValue() - 1);
    else
        ed->proc.loadPatchByName (name);
    ed->refreshBars();
}

// ---------------------------------------------------------------- Editor

F4RTAudioProcessorEditor::F4RTAudioProcessorEditor (F4RTAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&panelLnF);
    setOpaque (true);
    panelLnF.setColour (juce::PopupMenu::backgroundColourId, kInk);
    panelLnF.setColour (juce::PopupMenu::textColourId, kLight);
    panelLnF.setColour (juce::PopupMenu::highlightedBackgroundColourId, kGreen);
    panelLnF.setColour (juce::PopupMenu::highlightedTextColourId, kInk);
    panelLnF.setColour (juce::ComboBox::textColourId, kLight);
    panelLnF.setColour (juce::ComboBox::arrowColourId, kYellow);
    panelLnF.setColour (juce::TooltipWindow::backgroundColourId, kInk);
    panelLnF.setColour (juce::TooltipWindow::textColourId, kLight);
    panelLnF.setColour (juce::TooltipWindow::outlineColourId, kGreen);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    int page = Sound;
    auto addK = [this, &page] (const char* id, const juce::String& name, const juce::String& tip,
                               const juce::String& hint = {}) -> Knob*
    {
        auto* k = knobs.add (new Knob (proc.apvts, id, name, tip, hint));
        content.addChildComponent (k);
        pageItems[page].add (k);
        return k;
    };
    auto addC = [this, &page] (const char* id, const juce::String& name, const juce::String& tip) -> Combo*
    {
        auto* c = combos.add (new Combo (proc.apvts, id, name, tip));
        content.addChildComponent (c);
        pageItems[page].add (c);
        return c;
    };
    auto addT = [this, &page] (const char* id, const juce::String& name, const juce::String& tip) -> Toggle*
    {
        auto* t = toggles.add (new Toggle (proc.apvts, id, name, tip));
        content.addChildComponent (t);
        pageItems[page].add (t);
        return t;
    };

    page = Sound;
    for (int i = 0; i < 4; ++i)
    {
        const auto n = juce::String (i + 1);
        oType[i] = addC (ParamID::oType[i], "Type",
                         "Osc " + n + " waveform: sine, tri, saw, square, wavetable, noise or user WT.");
        oVol[i] = addK (ParamID::oVol[i], "Volume", "Osc " + n + " volume.");
        oPan[i] = addK (ParamID::oPan[i], "Pan", "Osc " + n + " pan (left to right).");
        oCoarse[i] = addK (ParamID::oCoarse[i], "Coarse", "Osc " + n + " pitch in semitones.");
        oFine[i] = addK (ParamID::oFine[i], "Fine", "Osc " + n + " pitch in cents.");
        oPhase[i] = addK (ParamID::oPhase[i], "Phase", "Osc " + n + " start phase.");
        oWidth[i] = addK (ParamID::oWidth[i], "Width", "Osc " + n + " stereo width.");
        for (int j = 0; j < 4; ++j)
            fm[i][j] = addK (ParamID::fm[i][j], {},
                             "How much osc " + n + " modulates osc " + juce::String (j + 1) + ".");
    }
    fmEnv = addK (ParamID::fmEnvDepth, "FM Env", "How much the FM envelope scales the FM amounts.");
    fmRecipe = addC (ParamID::fmRecipe, "Recipe", "Preset FM graph. Click Apply FM to write it.");
    formA = addC (ParamID::formA, "From", "Start vowel.");
    formB = addC (ParamID::formB, "To", "End vowel.");
    formMorph = addK (ParamID::formMorph, "Morph", "Blend From into To.");
    formTrack = addK (ParamID::formTrack, "Follow note", "How much the vowel frequencies follow the MIDI note.");
    formMix = addK (ParamID::formMix, "Mix", "Dry/wet of the vowel filter.");
    filtType = addC (ParamID::filtType, "Type", "LP, HP, BP, notch, comb. SVF is the same as LP.");
    cutoff = addK (ParamID::cutoff, "Cutoff", "Filter cutoff frequency.");
    res = addK (ParamID::res, "Resonance", "Filter resonance / emphasis.");
    filtDrive = addK (ParamID::filtDrive, "Drive in", "Saturation into the filter.");
    keyTrack = addK (ParamID::keyTrack, "Follow note", "How much cutoff follows the MIDI note.");
    filtEnv = addK (ParamID::filtEnv, "Env amount", "How much the filter envelope moves cutoff.");
    distType = addC (ParamID::distType, "Type", "Soft, hard, tube, fold, crush or digital.");
    distDrive = addK (ParamID::distDrive, "Drive", "Distortion amount.");
    distTone = addK (ParamID::distTone, "Tone", "Distortion brightness.");
    distMix = addK (ParamID::distMix, "Mix", "Dry/wet of the distortion.");
    static const char* macN[] = { "Size", "Wetness", "Anger", "Alienness", "Squelch", "Rip Length", "Toxicity" };
    static const char* macH[] = { "longer / darker", "more squelch", "FM + grit", "wobble vowel",
                                  "extra resonance", "tail", "fold when high" };
    static const char* macT[] = {
        "Amp decay and cutoff, baked when the note starts. Bigger = longer, darker.",
        "Resonance and formant mix. Wetter = squelchier.",
        "FM amount, FM envelope and distortion drive.",
        "Wobbles formant morph via LFO 1. Also a matrix source.",
        "Resonance. Higher = squelchier. Comb is a filter-type choice.",
        "Amp release, baked when the note starts. Longer rip = longer tail.",
        "Above about 0.55 switches distortion to fold. Also a matrix source."
    };
    for (int i = 0; i < 7; ++i)
        macros[i] = addK (ParamID::macro[i], macN[i], macT[i], macH[i]);
    voices = addK (ParamID::voices, "Voices", "How many notes at once (1-64).");
    polyMode = addC (ParamID::polyMode, "Mode", "Mono, legato or polyphonic.");
    glide = addK (ParamID::glide, "Glide", "Seconds to slide to the next note in mono and legato. Zero snaps. Poly stays instant.");
    bendRange = addC (ParamID::bendRange, "Bend", "Pitch-bend range: 2, 12 or 24 semitones. MPE is already per channel.");
    randMode = addC (ParamID::randMode, "Flavour", "Flavour for All / Safe randomize (Clean, Psytrance, Forest...).");

    page = Motion;
    static const char* envWhat[] = {
        "pitch", "filter cutoff", "FM amounts", "vowel morph", "distortion", "loudness"
    };
    for (int i = 0; i < 6; ++i)
    {
        const juce::String w = envWhat[i];
        envAtk[i] = addK (ParamID::eAtk[i], "Attack", "Attack of the " + w + " envelope.");
        envDec[i] = addK (ParamID::eDec[i], "Decay", "Decay of the " + w + " envelope.");
        envSus[i] = addK (ParamID::eSus[i], "Sustain", "Sustain of the " + w + " envelope.");
        envRel[i] = addK (ParamID::eRel[i], "Release", "Release of the " + w + " envelope.");
    }
    for (int i = 0; i < 4; ++i)
    {
        const auto n = juce::String (i + 1);
        lShape[i] = addC (ParamID::lShape[i], "Shape", "LFO " + n + " shape (sine, tri, ramp, random, S&H, user).");
        lRate[i] = addK (ParamID::lRate[i], "Rate", "LFO " + n + " rate in Hz (or tempo if Sync is on).");
        lAmt[i] = addK (ParamID::lAmt[i], "Amount", "LFO " + n + " amount.");
        lSync[i] = addT (ParamID::lSync[i], "Sync", "Lock LFO " + n + " rate to host tempo.");
    }

    page = Matrix;
    for (int i = 0; i < 16; ++i)
    {
        const auto n = juce::String (i + 1);
        mxOn[i] = addT (ParamID::mxOn[i], "On", "Lights route " + n + ". Does nothing until On.");
        mxSrc[i] = addC (ParamID::mxSrc[i], {}, "Modulation source for route " + n + ".");
        mxDst[i] = addC (ParamID::mxDst[i], {}, "What route " + n + " modulates.");
        mxAmt[i] = addK (ParamID::mxAmt[i], "Amount", "How much route " + n + " applies (negative inverts).");
    }

    page = Fx;
    eqOn = addT (ParamID::eqOn, "EQ", "Enable EQ. Knobs do nothing while off.");
    eqLo = addK (ParamID::eqLo, "Lo", "Low shelf gain.");
    eqMid = addK (ParamID::eqMid, "Gain", "Overall gain while EQ is on (not a mid band).");
    eqHi = addK (ParamID::eqHi, "Hi", "High shelf gain.");
    cmpOn = addT (ParamID::cmpOn, "Comp", "Enable compressor. Knobs do nothing while off.");
    cmpTh = addK (ParamID::cmpTh, "Threshold", "Level above which compression starts.");
    cmpRt = addK (ParamID::cmpRt, "Ratio", "How hard it squashes.");
    dlyOn = addT (ParamID::dlyOn, "Delay", "Enable delay. Knobs do nothing while off.");
    dlyT = addK (ParamID::dlyT, "Time", "Delay time.");
    dlyFb = addK (ParamID::dlyFb, "Feedback", "How much repeats feed back.");
    revOn = addT (ParamID::revOn, "Reverb", "Enable reverb. Knobs do nothing while off.");
    revSz = addK (ParamID::revSz, "Size", "Room size.");
    revDm = addK (ParamID::revDm, "Damp", "How fast highs die in the tail.");
    choOn = addT (ParamID::choOn, "Chorus", "Enable chorus. Knobs do nothing while off.");
    choRt = addK (ParamID::choRt, "Rate", "Chorus LFO rate.");
    choDp = addK (ParamID::choDp, "Depth", "Chorus LFO depth.");
    phOn = addT (ParamID::phOn, "Phaser", "Enable phaser. Knobs do nothing while off.");
    phRt = addK (ParamID::phRt, "Rate", "Phaser LFO rate.");
    phDp = addK (ParamID::phDp, "Depth", "Phaser LFO depth.");
    flOn = addT (ParamID::flOn, "Flanger", "Enable flanger. Knobs do nothing while off.");
    flRt = addK (ParamID::flRt, "Rate", "Flanger LFO rate.");
    flFb = addK (ParamID::flFb, "Feedback", "Flanger feedback (metallic when high).");
    shOn = addT (ParamID::shOn, "Ring shift", "Enable ring-mod shift. Knobs do nothing while off.");
    shHz = addK (ParamID::shHz, "Hz", "Shift in Hz (negative = down).");
    grOn = addT (ParamID::grOn, "Stutter", "Enable stutter delay. Knobs do nothing while off.");
    grSz = addK (ParamID::grSz, "Size", "Stutter grain length.");
    grDn = addK (ParamID::grDn, "Density", "How many grains overlap.");
    for (int i = 0; i < 9; ++i)
        fxSlot[i] = addC (ParamID::fxSlot[i], juce::String (i + 1),
                          "Which effect sits in slot " + juce::String (i + 1) + " of the chain. Left is first.");

    // ToggleButtons are only used for the LnF look here, so they must not latch.
    for (auto* b : { &playBtn, &randAll, &randSafe, &randFx, &randMut, &saveBtn, &loadBtn, &favBtn, &wtBtn, &recipeBtn })
    {
        b->setClickingTogglesState (false);
        content.addChildComponent (b);
    }

    playBtn.setTooltip ("Play a test toot (C3).");
    randAll.setTooltip ("Random new patch. Uses Rand Mode for flavour.");
    randSafe.setTooltip ("Random, tamer ranges. Uses Rand Mode.");
    randFx.setTooltip ("Random delay / reverb only. Leaves the rest.");
    randMut.setTooltip ("Nudge the current patch a little.");
    recipeBtn.setTooltip ("Write the Recipe menu into the FM grid. The menu alone does nothing.");
    wtBtn.setTooltip ("Load a wav file as the user wavetable (Osc type User WT).");
    saveBtn.setTooltip ("Save this patch as a .f4rtp in the library folder.");
    loadBtn.setTooltip ("Import a .f4rtp into the library and load it.");
    favBtn.setTooltip ("Toggle favourite on the current patch (star in Patches).");
    playBtn.onClick = [this] { proc.requestNote(); };
    randAll.onClick = [this] { proc.randomize (f4rt::randAll); };
    randSafe.onClick = [this] { proc.randomize (f4rt::randSafe); };
    randFx.onClick = [this] { proc.randomize (f4rt::randFxOnly); };
    randMut.onClick = [this] { proc.randomize (f4rt::randMutate); };
    saveBtn.onClick = [this] { chooseSaveAs(); };
    loadBtn.onClick = [this] { chooseImport(); };
    favBtn.onClick = [this] { proc.toggleFavorite (proc.patchName()); refreshBrowser(); };
    recipeBtn.onClick = [this] { applyFmRecipe(); };
    wtBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Wavetable", juce::File(), "*.wav;*.aif;*.aiff");
        juce::Component::SafePointer<F4RTAudioProcessorEditor> safe (this);
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [safe] (const juce::FileChooser& c)
                              {
                                  if (safe != nullptr && c.getResult().existsAsFile())
                                      safe->proc.importWavetable (c.getResult());
                              });
    };

    search.setTextToShowWhenEmpty ("search / tags", kGrey);
    search.setText (proc.searchText, juce::dontSendNotification);
    search.onTextChange = [this] { proc.searchText = search.getText(); refreshBrowser(); };
    search.setTooltip ("Filter factory and user patches by name or tag.");
    content.addChildComponent (search);
    browserModel.ed = this;
    browser.setModel (&browserModel);
    browser.setColour (juce::ListBox::backgroundColourId, kInk);
    browser.setTooltip ("Click a row to load it. * = favourite.");
    content.addChildComponent (browser);

    vizMode.addItemList ({ "Wave", "Spectrum", "Spectrogram", "FM", "Env", "Matrix" }, 1);
    vizMode.setSelectedId (1, juce::dontSendNotification);
    vizMode.onChange = [this] { viz.mode = vizMode.getSelectedId() - 1; viz.repaint(); };
    vizMode.setTooltip ("What the scope draws.");
    content.addChildComponent (vizMode);
    viz.setTooltip ("Live view of the current scope mode.");
    content.addChildComponent (viz);

    patchBar.onStep = [this] (int d) { cyclePatch (d); };
    patchBar.onOpen = [this] { openPatchMenu(); };
    bankBar.onStep = [this] (int d) { cycleBank (d); };
    bankBar.onOpen = [this] { openBankMenu(); };
    pageTabs.setTooltip ("Sound is the chain. Motion, Matrix, FX, Patches are extra.");
    patchBar.setTooltip ("Patch. Arrows step factory then user; click for save / load.");
    bankBar.setTooltip ("Category. Arrows jump to the first patch of the next category.");
    content.addAndMakeVisible (pageTabs);
    for (auto* c : { (juce::Component*) &patchBar, (juce::Component*) &bankBar,
                     (juce::Component*) &volumeStrip })
        content.addAndMakeVisible (c);
    refreshBars();

    content.addChildComponent (aboutBox);

    refreshBrowser();
    applyPageVisibility();

    baseW = kEditorW;
    baseH = kEditorH;
    content.setSize (baseW, baseH);
    addAndMakeVisible (content);

    // Drag the corner to scale (40%..150%), aspect ratio locked. The scale is saved with the session.
    // Read it first: setResizeLimits clamps the still-empty editor and resized() overwrites uiScale.
    const float s = juce::jlimit (0.4f, 1.5f, proc.uiScale);
    setResizable (true, true);
    setResizeLimits (baseW * 2 / 5, baseH * 2 / 5, baseW * 3 / 2, baseH * 3 / 2);
    getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
    setSize (juce::roundToInt ((float) baseW * s), juce::roundToInt ((float) baseH * s));
    startTimerHz (20);
}

F4RTAudioProcessorEditor::~F4RTAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void F4RTAudioProcessorEditor::timerCallback()
{
    refreshBars();
    volumeStrip.refresh();
    if (editorPage == Sound)
        viz.repaint();
}

void F4RTAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (kChassis);  // covers the sub-pixel edge the scaled content may leave
}

void F4RTAudioProcessorEditor::resized()
{
    if (baseW == 0)
        return;  // called before the constructor finished
    const float s = (float) getWidth() / (float) baseW;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, baseW, baseH);
    proc.uiScale = s;
}

void F4RTAudioProcessorEditor::paintContent (juce::Graphics& g)
{
    g.fillAll (kChassis);

    auto r = content.getLocalBounds().reduced (kPad);
    auto title = r.removeFromTop (kTitleH);
    if (logo.isValid())
    {
        auto logoR = title.removeFromLeft (kTitleH).reduced (2);
        g.drawImage (logo, logoR.toFloat(), juce::RectanglePlacement::centred);
        title.removeFromLeft (6);
    }
    auto nameCol = title.removeFromLeft (kTitleHitW - kTitleH - 6);
    g.setColour (kYellow);
    g.setFont (boldFont (16.f));
    g.drawText ("F4RT", nameCol.removeFromTop (22), juce::Justification::centredLeft, false);
    g.setColour (kLight);
    g.setFont (boldFont (10.f));
    g.drawText ("v" JucePlugin_VersionString, nameCol, juce::Justification::centredLeft, false);

    g.setColour (kChassis.brighter (0.08f));
    g.drawRoundedRectangle (content.getLocalBounds().toFloat().reduced (1.5f), 2.f, 2.f);

    for (const auto& s : sections)
    {
        auto b = s.bounds.toFloat();
        g.setColour (kChassis.brighter (0.12f));
        g.fillRoundedRectangle (b, 4.f);
        g.setColour (kGrey);
        g.drawRoundedRectangle (b.reduced (0.5f), 4.f, 1.f);
        auto t = s.bounds.reduced (8, 1).removeFromTop (kSectionHead);
        g.setColour (kYellow);
        g.setFont (boldFont (11.f));
        g.drawText (s.title, t, juce::Justification::centredLeft, true);
        if (s.sub.isNotEmpty())
        {
            g.setColour (kGrey);
            g.setFont (boldFont (10.f));
            g.drawText (s.sub, t, juce::Justification::centredRight, true);
        }
    }

    if (editorPage != Sound)
        return;

    g.setFont (boldFont (11.f));
    g.setColour (kYellow);
    for (int i = 0; i < 4; ++i)
        if (! oscLabels[i].isEmpty())
            g.drawText ("Osc " + juce::String (i + 1), oscLabels[i],
                        juce::Justification::centred, false);

    if (! fmGrid.isEmpty())
    {
        const int cw = juce::jmax (1, fmGrid.getWidth() / 4);
        const int ch = juce::jmax (1, fmGrid.getHeight() / 4);
        g.setFont (boldFont (9.f));
        g.setColour (kGrey);
        for (int j = 0; j < 4; ++j)
            g.drawText ("into " + juce::String (j + 1),
                        fmGrid.getX() + j * cw, fmGrid.getY() - 14, cw, 14,
                        juce::Justification::centred, false);
        for (int i = 0; i < 4; ++i)
            g.drawText (juce::String (i + 1),
                        fmGrid.getX() - 22, fmGrid.getY() + i * ch, 20, ch,
                        juce::Justification::centred, false);
    }

    g.setColour (kYellow);
    for (auto gap : chainGaps)
    {
        if (gap.getWidth() < 8)
            continue;
        juce::Path p;
        const float cx = (float) gap.getCentreX();
        const float cy = (float) gap.getCentreY();
        p.addTriangle (cx - 4.f, cy - 6.f, cx + 5.f, cy, cx - 4.f, cy + 6.f);
        g.fillPath (p);
    }
}

void F4RTAudioProcessorEditor::placeRow (juce::Rectangle<int> r, juce::Component* const* items, int n)
{
    if (n <= 0)
        return;
    const int w = juce::jmax (1, r.getWidth() / n);
    for (int i = 0; i < n; ++i)
        if (items[i] != nullptr)
            items[i]->setBounds (r.getX() + i * w, r.getY(), w, r.getHeight());
}

void F4RTAudioProcessorEditor::layoutSound (juce::Rectangle<int> r)
{
    const int h = r.getHeight();
    auto top = r.removeFromTop (h * 44 / 100);
    r.removeFromTop (5);
    auto chain = r.removeFromTop (h * 24 / 100);
    r.removeFromTop (5);
    auto scope = r.removeFromBottom (juce::jmax (64, h * 14 / 100));
    r.removeFromBottom (4);

    auto oscS = top.removeFromLeft (top.getWidth() * 62 / 100);
    auto fmS = top;
    sections.push_back ({ oscS, "Oscillators", "who makes the toot" });
    sections.push_back ({ fmS, "FM", "who bends whose pitch" });

    auto oscBody = oscS.reduced (4).withTrimmedTop (kSectionHead);
    wtBtn.setBounds (oscBody.removeFromTop (24).removeFromRight (100).reduced (2));
    oscBody.removeFromTop (2);
    const int orh = juce::jmax (1, oscBody.getHeight() / 4);
    for (int i = 0; i < 4; ++i)
    {
        auto row = oscBody.removeFromTop (orh);
        oscLabels[i] = row.removeFromLeft (52);
        juce::Component* cells[] = { oType[i], oVol[i], oPan[i], oCoarse[i], oFine[i], oPhase[i], oWidth[i] };
        placeRow (row, cells, 7);
    }

    auto fmBody = fmS.reduced (4).withTrimmedTop (kSectionHead);
    auto tools = fmBody.removeFromTop (52);
    recipeBtn.setBounds (tools.removeFromRight (88).reduced (2, 10));
    juce::Component* toolC[] = { fmRecipe, fmEnv };
    placeRow (tools, toolC, 2);
    fmBody.removeFromTop (2);
    fmBody.removeFromTop (14);
    fmBody.removeFromLeft (22);
    fmGrid = fmBody;
    const int fmcw = juce::jmax (1, fmGrid.getWidth() / 4);
    const int fmch = juce::jmax (1, fmGrid.getHeight() / 4);
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            fm[i][j]->setBounds (fmGrid.getX() + j * fmcw, fmGrid.getY() + i * fmch, fmcw, fmch);

    const int gap = 14;
    const int colW = juce::jmax (1, (chain.getWidth() - 2 * gap) / 3);
    auto vowel = chain.removeFromLeft (colW);
    chainGaps[0] = chain.removeFromLeft (gap);
    auto filt = chain.removeFromLeft (colW);
    chainGaps[1] = chain.removeFromLeft (gap);
    auto grit = chain;
    sections.push_back ({ vowel, "Vowel", "mouth" });
    sections.push_back ({ filt, "Filter", "squelch" });
    sections.push_back ({ grit, "Grit", "distortion" });
    juce::Component* v[] = { formA, formB, formMorph, formTrack, formMix };
    juce::Component* f[] = { filtType, cutoff, res, filtDrive, keyTrack, filtEnv };
    juce::Component* d[] = { distType, distDrive, distTone, distMix };
    placeRow (vowel.reduced (4).withTrimmedTop (kSectionHead), v, 5);
    placeRow (filt.reduced (4).withTrimmedTop (kSectionHead), f, 6);
    placeRow (grit.reduced (4).withTrimmedTop (kSectionHead), d, 4);

    auto charS = r.removeFromLeft (r.getWidth() * 62 / 100);
    auto voiceS = r;
    sections.push_back ({ charS, "Character", "twist the sound from here" });
    sections.push_back ({ voiceS, "Voice", "how notes live" });
    juce::Component* mac[] = { macros[0], macros[1], macros[2], macros[3], macros[4], macros[5], macros[6] };
    placeRow (charS.reduced (4).withTrimmedTop (kSectionHead), mac, 7);

    auto vb = voiceS.reduced (4).withTrimmedTop (kSectionHead);
    auto btns = vb.removeFromTop (28);
    playBtn.setBounds (btns.removeFromLeft (64).reduced (2));
    for (auto* b : { &randAll, &randSafe, &randFx, &randMut })
        b->setBounds (btns.removeFromLeft (64).reduced (2));
    juce::Component* vc[] = { voices, polyMode, glide, bendRange, randMode };
    placeRow (vb, vc, 5);

    sections.push_back ({ scope, "Scope", {} });
    vizMode.setBounds (scope.removeFromTop (22).removeFromLeft (160).reduced (2, 1));
    viz.setBounds (scope.reduced (4, 2));
}

void F4RTAudioProcessorEditor::layoutMotion (juce::Rectangle<int> r)
{
    static const char* envTitle[] = { "Pitch", "Filter", "FM", "Vowel", "Grit", "Loudness" };
    static const char* envSub[] = { "always on", "moves cutoff", "scales FM", "morphs vowel",
                                    "pushes drive", "sustain near 0 = one-shot" };
    auto envArea = r.removeFromTop (r.getHeight() * 62 / 100);
    r.removeFromTop (6);
    const int cw = juce::jmax (1, envArea.getWidth() / 3);
    const int ch = juce::jmax (1, envArea.getHeight() / 2);
    for (int i = 0; i < 6; ++i)
    {
        auto card = juce::Rectangle<int> (envArea.getX() + (i % 3) * cw,
                                          envArea.getY() + (i / 3) * ch, cw, ch).reduced (3);
        sections.push_back ({ card, envTitle[i], envSub[i] });
        juce::Component* e[] = { envAtk[i], envDec[i], envSus[i], envRel[i] };
        placeRow (card.reduced (4).withTrimmedTop (kSectionHead), e, 4);
    }

    const int lw = juce::jmax (1, r.getWidth() / 4);
    for (int i = 0; i < 4; ++i)
    {
        auto card = juce::Rectangle<int> (r.getX() + i * lw, r.getY(), lw, r.getHeight()).reduced (3);
        sections.push_back ({ card, "LFO " + juce::String (i + 1), {} });
        juce::Component* l[] = { lShape[i], lRate[i], lAmt[i], lSync[i] };
        placeRow (card.reduced (4).withTrimmedTop (kSectionHead), l, 4);
    }
}

void F4RTAudioProcessorEditor::layoutMatrix (juce::Rectangle<int> r)
{
    sections.push_back ({ r, "Cables", "On  |  From  →  To  |  Amount   ·   16 of 32" });
    auto body = r.reduced (8).withTrimmedTop (kSectionHead);
    const int onW = 70, amtW = 110;
    const int rh = juce::jmax (1, body.getHeight() / 16);
    for (int i = 0; i < 16; ++i)
    {
        auto row = body.removeFromTop (rh);
        mxOn[i]->setBounds (row.removeFromLeft (onW));
        mxAmt[i]->setBounds (row.removeFromRight (amtW));
        const int half = row.getWidth() / 2;
        mxSrc[i]->setBounds (row.removeFromLeft (half));
        mxDst[i]->setBounds (row);
    }
}

void F4RTAudioProcessorEditor::layoutFx (juce::Rectangle<int> r)
{
    auto orderS = r.removeFromBottom (100);
    r.removeFromBottom (6);
    sections.push_back ({ orderS, "Order", "left is first" });
    juce::Component* slots[] = { fxSlot[0], fxSlot[1], fxSlot[2], fxSlot[3], fxSlot[4],
                                 fxSlot[5], fxSlot[6], fxSlot[7], fxSlot[8] };
    placeRow (orderS.reduced (4).withTrimmedTop (kSectionHead), slots, 9);

    Toggle* on[9] = { eqOn, cmpOn, dlyOn, revOn, choOn, phOn, flOn, shOn, grOn };
    Knob* ka[9][3] = {
        { eqLo, eqMid, eqHi }, { cmpTh, cmpRt, nullptr }, { dlyT, dlyFb, nullptr },
        { revSz, revDm, nullptr }, { choRt, choDp, nullptr }, { phRt, phDp, nullptr },
        { flRt, flFb, nullptr }, { shHz, nullptr, nullptr }, { grSz, grDn, nullptr }
    };
    const int cw = juce::jmax (1, r.getWidth() / 3);
    const int ch = juce::jmax (1, r.getHeight() / 3);
    for (int i = 0; i < 9; ++i)
    {
        auto card = juce::Rectangle<int> (r.getX() + (i % 3) * cw,
                                          r.getY() + (i / 3) * ch, cw, ch).reduced (3);
        sections.push_back ({ card, {}, {} });
        auto body = card.reduced (4);
        on[i]->setBounds (body.removeFromLeft (88));
        juce::Component* ks[3] = { ka[i][0], ka[i][1], ka[i][2] };
        int n = 0;
        while (n < 3 && ks[n] != nullptr)
            ++n;
        if (n > 0)
            placeRow (body, ks, n);
    }
}

void F4RTAudioProcessorEditor::layoutPatches (juce::Rectangle<int> r)
{
    sections.push_back ({ r, "Patches", "200 factory seeds, then your saves" });
    auto body = r.reduced (8).withTrimmedTop (kSectionHead);
    auto row = body.removeFromTop (28);
    saveBtn.setBounds (row.removeFromLeft (84).reduced (2));
    loadBtn.setBounds (row.removeFromLeft (84).reduced (2));
    favBtn.setBounds (row.removeFromLeft (64).reduced (2));
    search.setBounds (row.reduced (2));
    body.removeFromTop (4);
    browser.setBounds (body);
}

void F4RTAudioProcessorEditor::layoutContent()
{
    sections.clear();
    fmGrid = {};
    chainGaps[0] = {};
    chainGaps[1] = {};
    for (auto& o : oscLabels)
        o = {};

    auto r = content.getLocalBounds().reduced (kPad);
    auto title = r.removeFromTop (kTitleH);
    title.removeFromLeft (kTitleHitW);
    volumeStrip.setBounds (title.removeFromRight (240).reduced (2, 4));
    const int barW = title.getWidth() / 2;
    patchBar.setBounds (title.removeFromLeft (barW).reduced (4, 4));
    bankBar.setBounds (title.reduced (4, 4));
    r.removeFromTop (4);
    pageTabs.setBounds (r.removeFromTop (kTabH));
    r.removeFromTop (4);

    switch (editorPage)
    {
        case Sound:   layoutSound (r);   break;
        case Motion:  layoutMotion (r);  break;
        case Matrix:  layoutMatrix (r);  break;
        case Fx:      layoutFx (r);      break;
        case Patches: layoutPatches (r); break;
        default: break;
    }

    aboutBox.setBounds (content.getLocalBounds());
}

void F4RTAudioProcessorEditor::applyPageVisibility()
{
    for (int p = 0; p < kPageCount; ++p)
        for (auto* c : pageItems[p])
            if (c != nullptr)
                c->setVisible (false);

    for (auto* b : { &playBtn, &randAll, &randSafe, &randFx, &randMut, &saveBtn, &loadBtn,
                     &favBtn, &wtBtn, &recipeBtn })
        b->setVisible (false);
    search.setVisible (false);
    browser.setVisible (false);
    vizMode.setVisible (false);
    viz.setVisible (false);

    for (auto* c : pageItems[editorPage])
        if (c != nullptr)
            c->setVisible (true);

    const bool sound = editorPage == Sound;
    playBtn.setVisible (sound);
    randAll.setVisible (sound);
    randSafe.setVisible (sound);
    randFx.setVisible (sound);
    randMut.setVisible (sound);
    recipeBtn.setVisible (sound);
    wtBtn.setVisible (sound);
    viz.setVisible (sound);
    vizMode.setVisible (sound);
    viz.mode = vizMode.getSelectedId() - 1;

    const bool patches = editorPage == Patches;
    saveBtn.setVisible (patches);
    loadBtn.setVisible (patches);
    favBtn.setVisible (patches);
    search.setVisible (patches);
    browser.setVisible (patches);

    layoutContent();
    pageTabs.repaint();
    if (aboutBox.isVisible())
        aboutBox.toFront (false);
}

void F4RTAudioProcessorEditor::setEditorPage (int page)
{
    editorPage = juce::jlimit (0, kPageCount - 1, page);
    applyPageVisibility();
}

void F4RTAudioProcessorEditor::contentMouseUp (const juce::MouseEvent& e)
{
    if (e.mouseWasClicked() && titleHitBounds().contains (e.getPosition()))
        openAbout();
}

juce::Rectangle<int> F4RTAudioProcessorEditor::titleHitBounds() const
{
    return { kPad, kPad, kTitleHitW, kTitleH };
}

void F4RTAudioProcessorEditor::openAbout()
{
    aboutBox.setVisible (true);
    aboutBox.toFront (true);
}

// ---------------------------------------------------------------- Patches
//
// One flat list for the arrows: 200 factory seeds (10 categories x 20), then user .f4rtp files.

void F4RTAudioProcessorEditor::refreshBars()
{
    const int fi = proc.factoryIndex();
    const auto name = proc.patchName();
    patchBar.setText (name);
    bankBar.setText (fi >= 0 ? juce::String (f4rt::kCats[fi / kCatSize])
                             : (name == "Init Patch" ? juce::String ("Init") : juce::String ("User")));
}

void F4RTAudioProcessorEditor::cyclePatch (int delta)
{
    const auto user = proc.patchNames();
    const int total = f4rt::kFactoryCount + user.size();
    int pos = proc.factoryIndex();
    if (pos < 0)
    {
        const int u = user.indexOf (proc.patchName());
        pos = u >= 0 ? f4rt::kFactoryCount + u : -1;
    }
    pos = pos < 0 ? (delta > 0 ? 0 : total - 1) : (pos + delta + total) % total;
    if (pos < f4rt::kFactoryCount)
        proc.loadFactory (pos);
    else
        proc.loadPatchByName (user[pos - f4rt::kFactoryCount]);
    refreshBars();
}

void F4RTAudioProcessorEditor::cycleBank (int delta)
{
    const int fi = proc.factoryIndex();
    const int cat = fi < 0 ? (delta > 0 ? 0 : kCatCount - 1)
                           : (fi / kCatSize + delta + kCatCount) % kCatCount;
    proc.loadFactory (cat * kCatSize);
    refreshBars();
}

void F4RTAudioProcessorEditor::openPatchMenu()
{
    const auto user = proc.patchNames();
    const auto cur = proc.patchName();
    const int fi = proc.factoryIndex();

    juce::PopupMenu m;
    m.setLookAndFeel (&panelLnF);  // menus are top-level windows and would otherwise use the default look
    m.addItem (kMenuInit, "Init Patch", true, cur == "Init Patch");
    m.addSeparator();
    m.addItem (kMenuSave, "Save");
    m.addItem (kMenuSaveAs, "Save As...");
    m.addItem (kMenuExport, "Export...");
    m.addItem (kMenuImport, "Import...");
    m.addSeparator();
    for (int c = 0; c < kCatCount; ++c)
    {
        juce::PopupMenu sub;
        for (int i = c * kCatSize; i < (c + 1) * kCatSize; ++i)
            sub.addItem (kMenuFactory + i, factoryLabel (i), true, i == fi);
        m.addSubMenu (f4rt::kCats[c], sub, true, nullptr, fi >= 0 && fi / kCatSize == c);
    }
    if (! user.isEmpty())
    {
        juce::PopupMenu sub;
        for (int i = 0; i < user.size(); ++i)
            sub.addItem (kMenuUser + i, user[i], true, fi < 0 && user[i] == cur);
        m.addSeparator();
        m.addSubMenu ("User", sub, true, nullptr, fi < 0 && user.contains (cur));
    }

    juce::Component::SafePointer<F4RTAudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (patchBar),
                     [safe, user] (int result)
                     {
                         if (safe == nullptr || result <= 0)
                             return;
                         auto& pr = safe->proc;
                         if (result == kMenuInit)
                             pr.initPatch();
                         else if (result == kMenuSave)
                         {
                             // Factory and Init have no file yet: ask for a name.
                             if (pr.factoryIndex() >= 0 || ! pr.savePatch())
                                 safe->chooseSaveAs();
                         }
                         else if (result == kMenuSaveAs)
                             safe->chooseSaveAs();
                         else if (result == kMenuExport)
                             safe->chooseExport();
                         else if (result == kMenuImport)
                             safe->chooseImport();
                         else if (result >= kMenuUser)
                         {
                             if (juce::isPositiveAndBelow (result - kMenuUser, user.size()))
                                 pr.loadPatchByName (user[result - kMenuUser]);
                         }
                         else if (result >= kMenuFactory)
                             pr.loadFactory (result - kMenuFactory);
                         safe->refreshBars();
                     });
}

void F4RTAudioProcessorEditor::openBankMenu()
{
    const int fi = proc.factoryIndex();
    juce::PopupMenu m;
    m.setLookAndFeel (&panelLnF);  // menus are top-level windows and would otherwise use the default look
    for (int c = 0; c < kCatCount; ++c)
        m.addItem (c + 1, f4rt::kCats[c], true, fi >= 0 && fi / kCatSize == c);
    juce::Component::SafePointer<F4RTAudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (bankBar),
                     [safe] (int result)
                     {
                         if (safe == nullptr || result <= 0)
                             return;
                         safe->proc.loadFactory ((result - 1) * kCatSize);
                         safe->refreshBars();
                     });
}

void F4RTAudioProcessorEditor::chooseSaveAs()
{
    const auto start = proc.patchesDir().getChildFile (proc.patchName() + ".f4rtp");
    chooser = std::make_unique<juce::FileChooser> ("Save Patch", start, "*.f4rtp");
    juce::Component::SafePointer<F4RTAudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& c)
                          {
                              if (safe == nullptr)
                                  return;
                              const auto file = c.getResult();
                              if (file.getFullPathName().isEmpty())
                                  return;
                              // Always lands in the library folder so it shows up in the patch list.
                              safe->proc.savePatchAs (file.getFileNameWithoutExtension());
                              safe->refreshBars();
                              safe->refreshBrowser();
                          });
}

void F4RTAudioProcessorEditor::chooseExport()
{
    const auto start = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                           .getChildFile (proc.patchName() + ".f4rtp");
    chooser = std::make_unique<juce::FileChooser> ("Export Patch", start, "*.f4rtp");
    juce::Component::SafePointer<F4RTAudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode
                              | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& c)
                          {
                              if (safe == nullptr)
                                  return;
                              const auto file = c.getResult();
                              if (file.getFullPathName().isNotEmpty())
                                  safe->proc.exportPatch (file);
                          });
}

void F4RTAudioProcessorEditor::chooseImport()
{
    chooser = std::make_unique<juce::FileChooser> (
        "Import Patch", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.f4rtp");
    juce::Component::SafePointer<F4RTAudioProcessorEditor> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& c)
                          {
                              if (safe == nullptr)
                                  return;
                              const auto file = c.getResult();
                              if (! file.existsAsFile())
                                  return;
                              safe->proc.importPatch (file);
                              safe->refreshBars();
                              safe->refreshBrowser();
                          });
}

void F4RTAudioProcessorEditor::applyFmRecipe()
{
    auto p = proc.currentParams();
    f4rt::applyFmPreset (p, juce::roundToInt (proc.apvts.getRawParameterValue (ParamID::fmRecipe)->load()));
    proc.writeParams (p);
}

void F4RTAudioProcessorEditor::refreshBrowser()
{
    browserModel.rows.clear();
    const auto q = proc.searchText.trim().toLowerCase();
    const auto favs = proc.favoriteNames();
    for (int i = 0; i < f4rt::kFactoryCount; ++i)
    {
        const auto name = factoryLabel (i);
        juce::String row = juce::String (i + 1).paddedLeft ('0', 3) + " | " + name + "  [" + f4rt::factoryAt (i).tags + "]";
        if (favs.contains (name))
            row += " *";
        if (q.isEmpty() || row.toLowerCase().contains (q))
            browserModel.rows.add (row);
    }
    for (const auto& n : proc.patchNames())
        if (q.isEmpty() || n.toLowerCase().contains (q))
            browserModel.rows.add (favs.contains (n) ? n + " *" : n);
    browser.updateContent();
    browser.repaint();
}
