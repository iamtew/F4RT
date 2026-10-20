#pragma once

#include "PluginProcessor.h"
#include "ui/Theme.h"

#include <atomic>
#include <functional>
#include <vector>

class F4RTAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    explicit F4RTAudioProcessorEditor (F4RTAudioProcessor&);
    ~F4RTAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Content : public juce::Component
    {
        explicit Content (F4RTAudioProcessorEditor& e) : ed (e) {}
        void paint (juce::Graphics& g) override { ed.paintContent (g); }
        void resized() override { ed.layoutContent(); }
        void mouseUp (const juce::MouseEvent& e) override { ed.contentMouseUp (e); }
        F4RTAudioProcessorEditor& ed;
    };
    void paintContent (juce::Graphics&);
    void layoutContent();
    void contentMouseUp (const juce::MouseEvent&);
    void timerCallback() override;
    juce::Rectangle<int> titleHitBounds() const;
    void openAbout();
    void setEditorPage (int page);
    void applyPageVisibility();
    void refreshBrowser();
    void applyFmRecipe();

    F4RTAudioProcessor& proc;

    struct PanelLnF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
        juce::Font getLabelFont (juce::Label&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
    };

    struct AboutBox : public juce::Component
    {
        AboutBox();
        void paint (juce::Graphics& g) override;
        void resized() override;
        void mouseUp (const juce::MouseEvent& e) override;
        void checkForUpdate();
        void applyResult (const juce::String& text, bool hasUpdate);
        juce::Rectangle<int> cardBounds() const;
        juce::Label status;
        juce::ToggleButton checkBtn { "Check for update" };
        juce::HyperlinkButton releasesBtn {
            "Open releases",
            juce::URL ("https://github.com/iamtew/F4RT/releases/latest")
        };
        juce::ToggleButton closeBtn { "Close" };
        std::atomic<bool> fetching { false };
    };

    struct Knob : public juce::Component
    {
        Knob (juce::AudioProcessorValueTreeState& st, const juce::String& id, const juce::String& name);
        void resized() override;
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    struct Combo : public juce::Component
    {
        Combo (juce::AudioProcessorValueTreeState& st, const juce::String& id, const juce::String& name);
        void resized() override;
        juce::Label label;
        juce::ComboBox box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
    };

    struct Viz : public juce::Component
    {
        explicit Viz (F4RTAudioProcessorEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        F4RTAudioProcessorEditor& ed;
        int mode = 0;
    };

    PanelLnF panelLnF;
    Content content { *this };
    int baseW = 0, baseH = 0;
    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<Combo> combos;
    juce::ToggleButton playBtn { "Play" };
    juce::ToggleButton randAll { "All" };
    juce::ToggleButton randSafe { "Safe" };
    juce::ToggleButton randFx { "FX" };
    juce::ToggleButton randMut { "Mutate" };
    juce::ToggleButton saveBtn { "Save" };
    juce::ToggleButton loadBtn { "Load" };
    juce::ToggleButton favBtn { "Fav" };
    juce::ToggleButton wtBtn { "Import WT" };
    juce::ToggleButton recipeBtn { "Apply FM" };
    juce::TextEditor search;
    juce::ListBox browser;
    struct BrowserModel : public juce::ListBoxModel
    {
        F4RTAudioProcessorEditor* ed = nullptr;
        juce::StringArray rows;
        int getNumRows() override { return rows.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override;
        void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    };
    BrowserModel browserModel;
    juce::ComboBox pageBox;
    juce::ComboBox vizMode;
    Viz viz { *this };
    AboutBox aboutBox;
    int editorPage = 0;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Image logo;
    juce::TooltipWindow tip { this };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (F4RTAudioProcessorEditor)
};
