#include "PluginEditor.h"
#include "parameters/FactoryBank.h"
#include "ui/VersionCompare.h"

#include <cmath>
#include <thread>

namespace
{
constexpr int kEditorW = 860;
constexpr int kEditorH = 540;
constexpr const char* kPageNames[] = { "Play", "Osc", "Filter", "Motion", "Matrix", "FX", "Browser", "Viz" };
constexpr int kPad = 8;
constexpr int kTitleH = 40;
constexpr int kTitleHitW = 150;
constexpr int kLabelH = 12;
constexpr int kBtnRowH = 28;
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
                                      const juce::String& tip)
{
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, kLight);
    addAndMakeVisible (label);
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
    label.setBounds (r.removeFromTop (kLabelH));
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
    label.setBounds (r.removeFromTop (kLabelH));
    box.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 4, 120), 24));
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
    btn.setBounds (getLocalBounds().withSizeKeepingCentre (juce::jmin (getWidth() - 4, 72), 24));
}

// ---------------------------------------------------------------- Look

void F4RTAudioProcessorEditor::PanelLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                           float pos, float startAngle, float endAngle,
                                                           juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.f);
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
    auto rc = bounds.withSizeKeepingCentre (d, d);
    g.setColour (kInk);
    g.fillEllipse (rc);
    auto cap = rc.reduced (d * 0.16f);
    g.setColour (kGrey);
    g.fillEllipse (cap);
    g.setColour (kLight);
    g.fillEllipse (cap.translated (-d * 0.06f, -d * 0.08f).withSizeKeepingCentre (cap.getWidth() * 0.45f,
                                                                                 cap.getHeight() * 0.35f));
    const float angle = startAngle + pos * (endAngle - startAngle);
    const auto c = cap.getCentre();
    const auto needle = c.getPointOnCircumference (cap.getWidth() * 0.42f, angle);
    g.setColour (kGreen);
    g.drawLine (c.x, c.y, needle.x, needle.y, 2.2f);
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
            g.drawText ("No active routes. Switch a row On on the Matrix page.", r.toNearestInt(),
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

    // Each add* creates the control and appends it to a page's grid, in reading order.
    int page = 0;
    auto addK = [this, &page] (const char* id, const juce::String& name, const juce::String& tip)
    {
        auto* k = knobs.add (new Knob (proc.apvts, id, name, tip));
        content.addChildComponent (k);
        pageItems[page].add (k);
    };
    auto addC = [this, &page] (const char* id, const juce::String& name, const juce::String& tip)
    {
        auto* c = combos.add (new Combo (proc.apvts, id, name, tip));
        content.addChildComponent (c);
        pageItems[page].add (c);
    };
    auto addT = [this, &page] (const char* id, const juce::String& name, const juce::String& tip)
    {
        auto* t = toggles.add (new Toggle (proc.apvts, id, name, tip));
        content.addChildComponent (t);
        pageItems[page].add (t);
    };
    auto gap = [this, &page] (int n)
    {
        for (int i = 0; i < n; ++i)
            pageItems[page].add (nullptr);
    };

    // Play: 7 macros, then voice setup. 5 columns -> 2 rows.
    page = 0;
    pageCols[page] = 5;
    static const char* macN[] = { "Size", "Wetness", "Anger", "Alienness", "Squelch", "Rip Length", "Toxicity" };
    static const char* macT[] = {
        "Amp decay and cutoff. Bigger = longer, darker.",
        "Resonance and formant mix. Wetter = squelchier.",
        "FM amount, FM envelope and distortion drive.",
        "Wobbles formant morph via LFO 1. Also a matrix source.",
        "Resonance. Above about 0.7 switches the filter to comb.",
        "Amp release. Longer rip = longer tail.",
        "Above about 0.55 switches distortion to fold. Also a matrix source."
    };
    for (int i = 0; i < 7; ++i)
        addK (ParamID::macro[i], macN[i], macT[i]);
    addK (ParamID::voices, "Voices", "How many notes at once (1-64).");
    addC (ParamID::polyMode, "Poly", "Mono, legato or polyphonic.");
    addC (ParamID::randMode, "Rand Mode", "Flavour for All / Safe randomize (Clean, Psytrance, Forest...).");

    // Osc: one row per oscillator (type, 6 knobs, FM into osc 1..4), then FM env + recipe.
    page = 1;
    pageCols[page] = 11;
    for (int i = 0; i < 4; ++i)
    {
        const auto n = juce::String (i + 1);
        addC (ParamID::oType[i], "Osc" + n,
              "Osc " + n + " waveform: sine, tri, saw, square, wavetable, noise or user WT.");
        addK (ParamID::oVol[i], "Vol", "Osc " + n + " volume.");
        addK (ParamID::oPan[i], "Pan", "Osc " + n + " pan (left to right).");
        addK (ParamID::oCoarse[i], "Coarse", "Osc " + n + " pitch in semitones.");
        addK (ParamID::oFine[i], "Fine", "Osc " + n + " pitch in cents.");
        addK (ParamID::oPhase[i], "Phase", "Osc " + n + " start phase.");
        addK (ParamID::oWidth[i], "Width", "Osc " + n + " stereo width.");
        for (int j = 0; j < 4; ++j)
            addK (ParamID::fm[i][j], "FM" + n + ">" + juce::String (j + 1),
                  "How much osc " + n + " modulates osc " + juce::String (j + 1) + ".");
    }
    addK (ParamID::fmEnvDepth, "FM Env", "How much the FM envelope scales the FM amounts.");
    addC (ParamID::fmRecipe, "Recipe", "Preset FM graph. Click Apply FM to write it.");

    // Filter: formant row, filter row, distortion row.
    page = 2;
    pageCols[page] = 6;
    addC (ParamID::formA, "Form A", "First formant vowel.");
    addC (ParamID::formB, "Form B", "Second formant vowel.");
    addC (ParamID::vowel, "Vowel", "Quick vowel pick (also a parameter).");
    addK (ParamID::formMorph, "Morph", "Blend Form A into Form B.");
    addK (ParamID::formTrack, "Track", "How much formants follow the MIDI note.");
    addK (ParamID::formMix, "Mix", "Dry/wet of the formant filter.");
    addC (ParamID::filtType, "Filter", "LP, HP, BP, notch, comb or SVF.");
    addK (ParamID::cutoff, "Cutoff", "Filter cutoff frequency.");
    addK (ParamID::res, "Res", "Filter resonance / emphasis.");
    addK (ParamID::filtDrive, "Drive", "Saturation into the filter.");
    addK (ParamID::keyTrack, "Key", "How much cutoff follows the MIDI note.");
    addK (ParamID::filtEnv, "Env", "How much the filter envelope moves cutoff.");
    addC (ParamID::distType, "Dist", "Soft, hard, tube, fold, crush or digital.");
    addK (ParamID::distDrive, "Drive", "Distortion amount.");
    addK (ParamID::distTone, "Tone", "Distortion brightness.");
    addK (ParamID::distMix, "Mix", "Dry/wet of the distortion.");

    // Motion: two envelopes per row, then two LFOs per row.
    page = 3;
    pageCols[page] = 8;
    static const char* envN[] = { "Pitch", "Filt", "FM", "Form", "Dist", "Amp" };
    static const char* envWhat[] = {
        "pitch", "filter cutoff", "FM amounts", "formant morph", "distortion", "amplitude"
    };
    for (int i = 0; i < 6; ++i)
    {
        const juce::String w = envWhat[i];
        addK (ParamID::eAtk[i], juce::String (envN[i]) + " A", "Attack of the " + w + " envelope.");
        addK (ParamID::eDec[i], "D", "Decay of the " + w + " envelope.");
        addK (ParamID::eSus[i], "S", "Sustain of the " + w + " envelope.");
        addK (ParamID::eRel[i], "R", "Release of the " + w + " envelope.");
    }
    for (int i = 0; i < 4; ++i)
    {
        const auto n = juce::String (i + 1);
        addC (ParamID::lShape[i], "LFO" + n, "LFO " + n + " shape (sine, tri, ramp, random, S&H, user).");
        addK (ParamID::lRate[i], "Rate", "LFO " + n + " rate in Hz (or tempo if Sync is on).");
        addK (ParamID::lAmt[i], "Amt", "LFO " + n + " amount.");
        addT (ParamID::lSync[i], "Sync", "Lock LFO " + n + " rate to host tempo.");
    }

    // Matrix: first 8 of 32 routes, two per row. A route does nothing until it is On.
    page = 4;
    pageCols[page] = 8;
    for (int i = 0; i < 8; ++i)
    {
        const auto n = juce::String (i + 1);
        addT (ParamID::mxOn[i], "On " + n, "Lights route " + n + ". Does nothing until On.");
        addC (ParamID::mxSrc[i], "Src", "Modulation source for route " + n + ".");
        addC (ParamID::mxDst[i], "Dst", "What route " + n + " modulates.");
        addK (ParamID::mxAmt[i], "Amt", "How much route " + n + " applies (negative inverts).");
    }

    // FX: three effects per row, each "On + up to 3 knobs", then the 9 order slots.
    page = 5;
    pageCols[page] = 12;
    struct FxSpec
    {
        const char* on;
        const char* name;
        const char* onTip;
        const char* k[3];
        const char* kn[3];
        const char* kt[3];
    };
    static const FxSpec kFx[] = {
        { ParamID::eqOn,  "EQ",     "Enable 3-band EQ.",
          { ParamID::eqLo, ParamID::eqMid, ParamID::eqHi }, { "Lo", "Mid", "Hi" },
          { "Low shelf gain.", "Mid band gain.", "High shelf gain." } },
        { ParamID::cmpOn, "Comp",   "Enable compressor.",
          { ParamID::cmpTh, ParamID::cmpRt, nullptr }, { "Thresh", "Ratio", nullptr },
          { "Level above which compression starts.", "How hard it squashes.", nullptr } },
        { ParamID::dlyOn, "Delay",  "Enable delay. Knobs do nothing while off.",
          { ParamID::dlyT, ParamID::dlyFb, nullptr }, { "Time", "FB", nullptr },
          { "Delay time.", "How much repeats feed back.", nullptr } },
        { ParamID::revOn, "Reverb", "Enable reverb.",
          { ParamID::revSz, ParamID::revDm, nullptr }, { "Size", "Damp", nullptr },
          { "Room size.", "How fast highs die in the tail.", nullptr } },
        { ParamID::choOn, "Chorus", "Enable chorus.",
          { ParamID::choRt, ParamID::choDp, nullptr }, { "Rate", "Depth", nullptr },
          { "Chorus LFO rate.", "Chorus LFO depth.", nullptr } },
        { ParamID::phOn,  "Phaser", "Enable phaser.",
          { ParamID::phRt, ParamID::phDp, nullptr }, { "Rate", "Depth", nullptr },
          { "Phaser LFO rate.", "Phaser LFO depth.", nullptr } },
        { ParamID::flOn,  "Flanger","Enable flanger.",
          { ParamID::flRt, ParamID::flFb, nullptr }, { "Rate", "FB", nullptr },
          { "Flanger LFO rate.", "Flanger feedback (metallic when high).", nullptr } },
        { ParamID::shOn,  "Shift",  "Enable frequency shifter.",
          { ParamID::shHz, nullptr, nullptr }, { "Hz", nullptr, nullptr },
          { "Shift in Hz (negative = down).", nullptr, nullptr } },
        { ParamID::grOn,  "Grain",  "Enable grain delay.",
          { ParamID::grSz, ParamID::grDn, nullptr }, { "Size", "Dens", nullptr },
          { "Grain length.", "How many grains overlap.", nullptr } },
    };
    for (const auto& fx : kFx)
    {
        addT (fx.on, fx.name, fx.onTip);
        for (int k = 0; k < 3; ++k)
        {
            if (fx.k[k] != nullptr)
                addK (fx.k[k], fx.kn[k], fx.kt[k]);
            else
                gap (1);
        }
    }
    for (int i = 0; i < 9; ++i)
        addC (ParamID::fxSlot[i], "Slot " + juce::String (i + 1),
              "Which effect sits in slot " + juce::String (i + 1) + " of the chain.");

    pageCols[6] = 1;
    pageCols[7] = 1;

    // Button rows. ToggleButtons are only used for the LnF look here, so they must not latch.
    for (auto* b : { &playBtn, &randAll, &randSafe, &randFx, &randMut, &saveBtn, &loadBtn, &favBtn, &wtBtn, &recipeBtn })
    {
        b->setClickingTogglesState (false);
        content.addChildComponent (b);
    }
    pageButtons[0] = { &playBtn, &randAll, &randSafe, &randFx, &randMut, &recipeBtn, &wtBtn };
    pageButtons[1] = { &recipeBtn, &wtBtn };
    pageButtons[6] = { &saveBtn, &loadBtn, &favBtn };

    playBtn.setTooltip ("Play a test note (C3).");
    randAll.setTooltip ("Random new patch. Uses Rand Mode for flavour.");
    randSafe.setTooltip ("Random, tamer ranges. Uses Rand Mode.");
    randFx.setTooltip ("Random delay / reverb only. Leaves the rest.");
    randMut.setTooltip ("Nudge the current patch a little.");
    recipeBtn.setTooltip ("Write the Osc-page FM recipe into the oscillators.");
    wtBtn.setTooltip ("Load a wav file as the user wavetable (Osc type User WT).");
    saveBtn.setTooltip ("Save this patch as a .f4rtp in the library folder.");
    loadBtn.setTooltip ("Import a .f4rtp into the library and load it.");
    favBtn.setTooltip ("Toggle favourite on the current patch (star in the Browser).");
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
    vizMode.setTooltip ("What the Viz page draws.");
    content.addChildComponent (vizMode);
    viz.setTooltip ("Live view of the current Viz mode.");
    content.addChildComponent (viz);

    // Top bar.
    pageBar.onStep = [this] (int d) { cyclePage (d); };
    pageBar.onOpen = [this] { openPageMenu(); };
    patchBar.onStep = [this] (int d) { cyclePatch (d); };
    patchBar.onOpen = [this] { openPatchMenu(); };
    bankBar.onStep = [this] (int d) { cycleBank (d); };
    bankBar.onOpen = [this] { openBankMenu(); };
    pageBar.setTooltip ("Page. Arrows step, click the name for the list.");
    patchBar.setTooltip ("Patch. Arrows step factory then user; click for save / load.");
    bankBar.setTooltip ("Category. Arrows jump to the first patch of the next category.");
    for (auto* c : { (juce::Component*) &pageBar, (juce::Component*) &patchBar,
                     (juce::Component*) &bankBar, (juce::Component*) &volumeStrip })
        content.addAndMakeVisible (c);
    pageBar.setText (kPageNames[0]);
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
    if (editorPage == 7)
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
}

void F4RTAudioProcessorEditor::layoutContent()
{
    auto r = content.getLocalBounds().reduced (kPad);
    auto title = r.removeFromTop (kTitleH);
    title.removeFromLeft (kTitleHitW);
    volumeStrip.setBounds (title.removeFromRight (240).reduced (2, 4));
    pageBar.setBounds (title.removeFromLeft (88).reduced (4, 4));
    const int barW = title.getWidth() / 2;
    patchBar.setBounds (title.removeFromLeft (barW).reduced (4, 4));
    bankBar.setBounds (title.reduced (4, 4));
    r.removeFromTop (4);

    if (! pageButtons[editorPage].isEmpty())
    {
        auto row = r.removeFromTop (kBtnRowH);
        for (auto* b : pageButtons[editorPage])
            b->setBounds (row.removeFromLeft (84).reduced (2, 2));
        r.removeFromTop (4);
    }

    if (editorPage == 6)
    {
        search.setBounds (r.removeFromTop (24));
        r.removeFromTop (4);
        browser.setBounds (r);
    }
    else if (editorPage == 7)
    {
        vizMode.setBounds (r.removeFromTop (24).removeFromLeft (160));
        r.removeFromTop (4);
        viz.setBounds (r);
    }
    else
    {
        const auto& items = pageItems[editorPage];
        const int cols = juce::jmax (1, pageCols[editorPage]);
        const int rows = juce::jmax (1, (items.size() + cols - 1) / cols);
        const int cw = r.getWidth() / cols;
        const int rh = juce::jmin (r.getHeight() / rows, 130);  // sparse pages stay compact
        for (int i = 0; i < items.size(); ++i)
            if (items[i] != nullptr)
                items[i]->setBounds (r.getX() + (i % cols) * cw, r.getY() + (i / cols) * rh, cw, rh);
    }

    aboutBox.setBounds (content.getLocalBounds());
}

void F4RTAudioProcessorEditor::applyPageVisibility()
{
    for (int p = 0; p < kPageCount; ++p)
    {
        for (auto* c : pageItems[p])
            if (c != nullptr)
                c->setVisible (false);
        for (auto* c : pageButtons[p])
            c->setVisible (false);
    }
    for (auto* c : pageItems[editorPage])
        if (c != nullptr)
            c->setVisible (true);
    for (auto* c : pageButtons[editorPage])
        c->setVisible (true);
    search.setVisible (editorPage == 6);
    browser.setVisible (editorPage == 6);
    vizMode.setVisible (editorPage == 7);
    viz.setVisible (editorPage == 7);
    layoutContent();
    if (aboutBox.isVisible())
        aboutBox.toFront (false);
}

void F4RTAudioProcessorEditor::setEditorPage (int page)
{
    editorPage = juce::jlimit (0, kPageCount - 1, page);
    pageBar.setText (kPageNames[editorPage]);
    applyPageVisibility();
}

void F4RTAudioProcessorEditor::cyclePage (int delta)
{
    setEditorPage ((editorPage + delta + kPageCount) % kPageCount);
}

void F4RTAudioProcessorEditor::openPageMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&panelLnF);  // menus are top-level windows and would otherwise use the default look
    for (int i = 0; i < kPageCount; ++i)
        m.addItem (i + 1, kPageNames[i], true, i == editorPage);
    juce::Component::SafePointer<F4RTAudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (pageBar),
                     [safe] (int result)
                     {
                         if (safe != nullptr && result > 0)
                             safe->setEditorPage (result - 1);
                     });
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
