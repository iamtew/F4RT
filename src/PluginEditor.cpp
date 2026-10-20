#include "PluginEditor.h"
#include "parameters/FactoryBank.h"
#include "ui/VersionCompare.h"

#include <cmath>
#include <thread>

namespace
{
constexpr int kEditorW = 860;
constexpr int kEditorH = 520;
constexpr const char* kPageNames[] = { "Play", "Osc", "Filter", "Motion", "Matrix", "FX", "Browser", "Viz" };
constexpr int kPageCount = 8;
constexpr int kPad = 8;
constexpr int kTitleH = 40;
constexpr int kLabelH = 12;

using f4rt::kGreen;
using f4rt::kYellow;
using f4rt::kLight;
using f4rt::kGrey;
using f4rt::kInk;
using f4rt::kChassis;
using f4rt::boldFont;
} // namespace

F4RTAudioProcessorEditor::Knob::Knob (juce::AudioProcessorValueTreeState& st,
                                      const juce::String& id, const juce::String& name)
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
}

void F4RTAudioProcessorEditor::Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (kLabelH));
    slider.setBounds (r);
}

F4RTAudioProcessorEditor::Combo::Combo (juce::AudioProcessorValueTreeState& st,
                                        const juce::String& id, const juce::String& name)
{
    label.setText (name, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setColour (juce::Label::textColourId, kLight);
    addAndMakeVisible (label);
    addAndMakeVisible (box);
    att = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, id, box);
}

void F4RTAudioProcessorEditor::Combo::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (kLabelH));
    box.setBounds (r.reduced (2, 8));
}

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

void F4RTAudioProcessorEditor::PanelLnF::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool, bool)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (2.f);
    g.setColour (b.getToggleState() ? kGreen.withAlpha (0.85f) : kInk);
    g.fillRoundedRectangle (bounds, 3.f);
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
}

juce::Font F4RTAudioProcessorEditor::PanelLnF::getLabelFont (juce::Label&) { return boldFont (11.f); }
juce::Font F4RTAudioProcessorEditor::PanelLnF::getComboBoxFont (juce::ComboBox&) { return boldFont (11.f); }

F4RTAudioProcessorEditor::AboutBox::AboutBox()
{
    status.setJustificationType (juce::Justification::centred);
    status.setColour (juce::Label::textColourId, kLight);
    addAndMakeVisible (status);
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
        g.setColour (kLight);
        g.setFont (boldFont (11.f));
        auto mx = ed.proc.currentMatrix();
        int y = 8;
        for (int i = 0; i < f4rt::kMatrixSlots && y < (int) r.getHeight(); ++i)
        {
            if (! mx.row[i].on)
                continue;
            g.drawText ("src " + juce::String (mx.row[i].src) + " -> " + juce::String (mx.row[i].dest)
                            + "  " + juce::String (mx.row[i].amt, 2),
                        (int) r.getX() + 8, (int) r.getY() + y, (int) r.getWidth() - 16, 14,
                        juce::Justification::centredLeft, true);
            y += 16;
        }
    }
}

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
    const auto name = rows[row];
    if (name.contains (" | "))
    {
        const int idx = name.upToFirstOccurrenceOf (" |", false, false).getIntValue() - 1;
        ed->proc.loadFactory (idx);
    }
    else
        ed->proc.loadPatchByName (name);
}

F4RTAudioProcessorEditor::F4RTAudioProcessorEditor (F4RTAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&panelLnF);
    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    auto addK = [this] (const juce::String& id, const juce::String& name)
    {
        auto* k = knobs.add (new Knob (proc.apvts, id, name));
        content.addAndMakeVisible (k);
    };
    auto addC = [this] (const juce::String& id, const juce::String& name)
    {
        auto* c = combos.add (new Combo (proc.apvts, id, name));
        content.addAndMakeVisible (c);
    };

    for (int i = 0; i < 7; ++i)
        addK (ParamID::macro[i], ParamID::macro[i]);
    addK (ParamID::level, "Level");
    addK (ParamID::voices, "Voices");
    addC (ParamID::polyMode, "Poly");
    addC (ParamID::randMode, "Rand");

    for (int i = 0; i < 4; ++i)
    {
        addC (ParamID::oType[i], "Osc" + juce::String (i + 1));
        addK (ParamID::oVol[i], "Vol");
        addK (ParamID::oPan[i], "Pan");
        addK (ParamID::oCoarse[i], "Coarse");
        addK (ParamID::oFine[i], "Fine");
        addK (ParamID::oPhase[i], "Phase");
        addK (ParamID::oWidth[i], "Width");
        for (int j = 0; j < 4; ++j)
            addK (ParamID::fm[i][j], "FM" + juce::String (i + 1) + juce::String (j + 1));
    }
    addK (ParamID::fmEnvDepth, "FM Env");
    addC (ParamID::fmRecipe, "Recipe");

    addC (ParamID::formA, "Form A");
    addC (ParamID::formB, "Form B");
    addK (ParamID::formMorph, "Morph");
    addK (ParamID::formTrack, "Track");
    addK (ParamID::formMix, "Mix");
    addC (ParamID::vowel, "Vowel");
    addC (ParamID::filtType, "Filter");
    addK (ParamID::cutoff, "Cutoff");
    addK (ParamID::res, "Res");
    addK (ParamID::filtDrive, "F Drive");
    addK (ParamID::keyTrack, "Key");
    addK (ParamID::filtEnv, "F Env");
    addC (ParamID::distType, "Dist");
    addK (ParamID::distDrive, "D Drive");
    addK (ParamID::distTone, "Tone");
    addK (ParamID::distMix, "D Mix");

    static const char* envN[] = { "Pitch", "Filt", "FM", "Form", "Dist", "Amp" };
    for (int i = 0; i < 6; ++i)
    {
        addK (ParamID::eAtk[i], juce::String (envN[i]) + " A");
        addK (ParamID::eDec[i], "D");
        addK (ParamID::eSus[i], "S");
        addK (ParamID::eRel[i], "R");
    }
    for (int i = 0; i < 4; ++i)
    {
        addC (ParamID::lShape[i], "LFO" + juce::String (i + 1));
        addK (ParamID::lRate[i], "Rate");
        addK (ParamID::lAmt[i], "Amt");
    }

    for (int i = 0; i < 8; ++i)
    {
        addC (ParamID::mxSrc[i], "Src");
        addC (ParamID::mxDst[i], "Dst");
        addK (ParamID::mxAmt[i], "Amt");
    }

    addK (ParamID::eqLo, "EQ Lo");
    addK (ParamID::eqMid, "EQ Mid");
    addK (ParamID::eqHi, "EQ Hi");
    addK (ParamID::cmpTh, "Comp Th");
    addK (ParamID::cmpRt, "Ratio");
    addK (ParamID::dlyT, "Delay");
    addK (ParamID::dlyFb, "FB");
    addK (ParamID::revSz, "Rev");
    addK (ParamID::revDm, "Damp");
    addK (ParamID::choRt, "Cho R");
    addK (ParamID::choDp, "Cho D");
    addK (ParamID::phRt, "Ph R");
    addK (ParamID::phDp, "Ph D");
    addK (ParamID::flRt, "Fl R");
    addK (ParamID::flFb, "Fl FB");
    addK (ParamID::shHz, "Shift");
    addK (ParamID::grSz, "Grain");
    addK (ParamID::grDn, "Dens");
    for (int i = 0; i < 9; ++i)
        addC (ParamID::fxSlot[i], "Slot" + juce::String (i + 1));

    auto setupBtn = [this] (juce::ToggleButton& b)
    {
        b.setClickingTogglesState (false);
        content.addAndMakeVisible (b);
    };
    setupBtn (playBtn);
    playBtn.onClick = [this] { proc.requestNote(); };
    setupBtn (randAll);
    randAll.onClick = [this] { proc.randomize (f4rt::randAll); };
    setupBtn (randSafe);
    randSafe.onClick = [this] { proc.randomize (f4rt::randSafe); };
    setupBtn (randFx);
    randFx.onClick = [this] { proc.randomize (f4rt::randFxOnly); };
    setupBtn (randMut);
    randMut.onClick = [this] { proc.randomize (f4rt::randMutate); };
    setupBtn (saveBtn);
    saveBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Save patch", proc.patchesDir(), "*.f4rtp");
        chooser->launchAsync (juce::FileBrowserComponent::saveMode, [this] (const juce::FileChooser& c)
        {
            auto f = c.getResult();
            if (f.existsAsFile() || f.getFileName().isNotEmpty())
                proc.savePatchAs (f.getFileNameWithoutExtension());
        });
    };
    setupBtn (loadBtn);
    loadBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Load patch", proc.patchesDir(), "*.f4rtp");
        chooser->launchAsync (juce::FileBrowserComponent::openMode, [this] (const juce::FileChooser& c)
        {
            auto f = c.getResult();
            if (f.existsAsFile())
                proc.importPatch (f);
        });
    };
    setupBtn (favBtn);
    favBtn.onClick = [this] { proc.toggleFavorite (proc.patchName()); refreshBrowser(); };
    setupBtn (wtBtn);
    wtBtn.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Wavetable", juce::File(), "*.wav;*.aif;*.aiff");
        chooser->launchAsync (juce::FileBrowserComponent::openMode, [this] (const juce::FileChooser& c)
        {
            auto f = c.getResult();
            if (f.existsAsFile())
                proc.importWavetable (f);
        });
    };
    setupBtn (recipeBtn);
    recipeBtn.onClick = [this] { applyFmRecipe(); };

    search.setTextToShowWhenEmpty ("search / tags", kGrey);
    search.onTextChange = [this] { proc.searchText = search.getText(); refreshBrowser(); };
    content.addAndMakeVisible (search);
    browserModel.ed = this;
    browser.setModel (&browserModel);
    content.addAndMakeVisible (browser);

    for (int i = 0; i < kPageCount; ++i)
        pageBox.addItem (kPageNames[i], i + 1);
    pageBox.setSelectedId (1, juce::dontSendNotification);
    pageBox.onChange = [this] { setEditorPage (pageBox.getSelectedId() - 1); };
    content.addAndMakeVisible (pageBox);

    vizMode.addItem ("Wave", 1);
    vizMode.addItem ("Spectrum", 2);
    vizMode.addItem ("Spectrogram", 3);
    vizMode.addItem ("FM", 4);
    vizMode.addItem ("Env", 5);
    vizMode.addItem ("Matrix", 6);
    vizMode.setSelectedId (1);
    vizMode.onChange = [this] { viz.mode = vizMode.getSelectedId() - 1; viz.repaint(); };
    content.addAndMakeVisible (vizMode);
    content.addAndMakeVisible (viz);

    content.addChildComponent (aboutBox);
    aboutBox.setVisible (false);

    refreshBrowser();
    applyPageVisibility();

    baseW = kEditorW;
    baseH = kEditorH;
    content.setSize (baseW, baseH);
    addAndMakeVisible (content);
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
    viz.repaint();
    proc.uiScale = (float) getWidth() / (float) baseW;
}

void F4RTAudioProcessorEditor::paint (juce::Graphics&) {}

void F4RTAudioProcessorEditor::paintContent (juce::Graphics& g)
{
    g.fillAll (kChassis);
    g.setColour (kInk);
    g.fillRect (0, 0, baseW, kTitleH);
    if (logo.isValid())
        g.drawImage (logo, juce::Rectangle<float> (8.f, 6.f, 28.f, 28.f));
    g.setColour (kYellow);
    g.setFont (boldFont (18.f));
    g.drawText ("F4RT  v" JucePlugin_VersionString, 42, 0, 280, kTitleH, juce::Justification::centredLeft, false);
    g.setColour (kGreen);
    g.setFont (boldFont (12.f));
    g.drawText (proc.patchName(), 320, 0, 240, kTitleH, juce::Justification::centredLeft, false);
}

void F4RTAudioProcessorEditor::resized()
{
    if (baseW == 0)
        return;
    const float s = (float) getWidth() / (float) baseW;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, baseW, baseH);
}

void F4RTAudioProcessorEditor::layoutContent()
{
    auto r = juce::Rectangle<int> (0, 0, baseW, baseH);
    auto top = r.removeFromTop (kTitleH);
    pageBox.setBounds (top.removeFromRight (140).reduced (8, 8));
    r.reduce (kPad, kPad);

    auto grid = [&] (juce::Array<juce::Component*> items, int cols, juce::Rectangle<int> area)
    {
        if (items.isEmpty())
            return;
        const int rows = (items.size() + cols - 1) / cols;
        const int cw = area.getWidth() / cols;
        const int rh = area.getHeight() / juce::jmax (1, rows);
        for (int i = 0; i < items.size(); ++i)
            items[i]->setBounds (area.getX() + (i % cols) * cw, area.getY() + (i / cols) * rh, cw, rh);
    };

    juce::Array<juce::Component*> show;
    auto takeKnobs = [&] (int from, int n)
    {
        for (int i = 0; i < n; ++i)
            show.add (knobs[from + i]);
    };
    auto takeCombos = [&] (int from, int n)
    {
        for (int i = 0; i < n; ++i)
            show.add (combos[from + i]);
    };

    // Index map from construction order.
    // knobs: 0-6 macros, 7 level, 8 voices,
    // then 4*(vol pan coarse fine phase width + 4 fm) = 4*10 = 40 starting at 9
    // fmEnv 49
    // form morph track mix 50-52, cutoff res fdrive key fenv 53-57, ddrive tone dmix 58-60
    // env 6*4 = 24 starting 61, lfo rate/amt 4*2 = 8 starting 85
    // mx amt 8 starting 93
    // fx knobs 18 starting 101
    //
    // combos: poly, rand, 4 osc type, recipe, formA formB vowel filt dist,
    // 4 lfo shape, 8*2 mx src/dst, 9 fx slots

    playBtn.setBounds (0, 0, 0, 0);
    const bool play = editorPage == 0;
    const bool osc = editorPage == 1;
    const bool filt = editorPage == 2;
    const bool motion = editorPage == 3;
    const bool matrix = editorPage == 4;
    const bool fxPage = editorPage == 5;
    const bool brow = editorPage == 6;
    const bool vizP = editorPage == 7;

    auto btnRow = r.removeFromTop (play || brow ? 28 : 0);
    if (play)
    {
        playBtn.setBounds (btnRow.removeFromLeft (70).reduced (2, 2));
        randAll.setBounds (btnRow.removeFromLeft (70).reduced (2, 2));
        randSafe.setBounds (btnRow.removeFromLeft (70).reduced (2, 2));
        randFx.setBounds (btnRow.removeFromLeft (70).reduced (2, 2));
        randMut.setBounds (btnRow.removeFromLeft (80).reduced (2, 2));
        recipeBtn.setBounds (btnRow.removeFromLeft (90).reduced (2, 2));
        wtBtn.setBounds (btnRow.removeFromLeft (100).reduced (2, 2));
        takeKnobs (0, 9);
        takeCombos (0, 2);
        grid (show, 6, r);
    }
    else if (osc)
    {
        takeCombos (2, 5); // osc types + recipe
        takeKnobs (9, 41);
        grid (show, 8, r);
    }
    else if (filt)
    {
        takeCombos (7, 5); // formA B vowel filt dist
        takeKnobs (50, 11);
        grid (show, 6, r);
    }
    else if (motion)
    {
        takeKnobs (61, 32);
        takeCombos (12, 4);
        grid (show, 8, r);
    }
    else if (matrix)
    {
        takeCombos (16, 16);
        takeKnobs (93, 8);
        grid (show, 6, r);
    }
    else if (fxPage)
    {
        takeKnobs (101, 18);
        takeCombos (32, 9);
        grid (show, 7, r);
    }
    else if (brow)
    {
        saveBtn.setBounds (btnRow.removeFromLeft (70).reduced (2, 2));
        loadBtn.setBounds (btnRow.removeFromLeft (70).reduced (2, 2));
        favBtn.setBounds (btnRow.removeFromLeft (70).reduced (2, 2));
        search.setBounds (r.removeFromTop (24));
        r.removeFromTop (4);
        browser.setBounds (r);
    }
    else if (vizP)
    {
        vizMode.setBounds (r.removeFromTop (24).removeFromLeft (160));
        r.removeFromTop (4);
        viz.setBounds (r);
    }

    aboutBox.setBounds (0, 0, baseW, baseH);
}

void F4RTAudioProcessorEditor::applyPageVisibility()
{
    auto visPage = [this] (juce::Component& c, bool v) { c.setVisible (v); };
    visPage (playBtn, editorPage == 0);
    visPage (randAll, editorPage == 0);
    visPage (randSafe, editorPage == 0);
    visPage (randFx, editorPage == 0);
    visPage (randMut, editorPage == 0);
    visPage (recipeBtn, editorPage == 0 || editorPage == 1);
    visPage (wtBtn, editorPage == 0 || editorPage == 1);
    visPage (saveBtn, editorPage == 6);
    visPage (loadBtn, editorPage == 6);
    visPage (favBtn, editorPage == 6);
    visPage (search, editorPage == 6);
    visPage (browser, editorPage == 6);
    visPage (viz, editorPage == 7);
    visPage (vizMode, editorPage == 7);

    for (auto* k : knobs)
        k->setVisible (false);
    for (auto* c : combos)
        c->setVisible (false);

    auto showK = [this] (int from, int n)
    {
        for (int i = 0; i < n && from + i < knobs.size(); ++i)
            knobs[from + i]->setVisible (true);
    };
    auto showC = [this] (int from, int n)
    {
        for (int i = 0; i < n && from + i < combos.size(); ++i)
            combos[from + i]->setVisible (true);
    };

    if (editorPage == 0) { showK (0, 9); showC (0, 2); }
    if (editorPage == 1) { showC (2, 5); showK (9, 41); }
    if (editorPage == 2) { showC (7, 5); showK (50, 11); }
    if (editorPage == 3) { showK (61, 32); showC (12, 4); }
    if (editorPage == 4) { showC (16, 16); showK (93, 8); }
    if (editorPage == 5) { showK (101, 18); showC (32, 9); }
    layoutContent();
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
    return { 0, 0, 300, kTitleH };
}

void F4RTAudioProcessorEditor::openAbout()
{
    aboutBox.setVisible (true);
    aboutBox.toFront (true);
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
    for (int i = 0; i < f4rt::kFactoryCount; ++i)
    {
        char name[64];
        f4rt::factoryName (i, name, 64);
        const auto e = f4rt::factoryAt (i);
        juce::String row = juce::String (i + 1).paddedLeft ('0', 3) + " | " + name + "  [" + e.tags + "]";
        if (proc.isFavorite (name))
            row += " *";
        if (q.isEmpty() || row.toLowerCase().contains (q) || juce::String (e.tags).contains (q)
            || juce::String (e.cat).toLowerCase().contains (q))
            browserModel.rows.add (row);
    }
    for (const auto& n : proc.patchNames())
        if (q.isEmpty() || n.toLowerCase().contains (q))
            browserModel.rows.add (n);
    browser.updateContent();
}
