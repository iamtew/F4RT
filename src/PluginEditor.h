#pragma once

#include "PluginProcessor.h"
#include "ui/Theme.h"

#include <atomic>
#include <functional>
#include <vector>

// Title row (About, page / patch / bank bars, volume strip) over eight pages.
// Same top-row layout as K1X, 5HKR and TEW03.
class F4RTAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    explicit F4RTAudioProcessorEditor (F4RTAudioProcessor&);
    ~F4RTAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int kPageCount = 8;

private:
    // Everything lives in `content`, laid out at a fixed base size and scaled as
    // one picture. Dragging the corner changes only the scale, so nothing reflows.
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
    void cyclePage (int delta);
    void openPageMenu();
    void applyPageVisibility();

    void refreshBars();
    void cyclePatch (int delta);
    void cycleBank (int delta);
    void openPatchMenu();
    void openBankMenu();
    void chooseSaveAs();
    void chooseExport();
    void chooseImport();

    void refreshBrowser();
    void applyFmRecipe();

    F4RTAudioProcessor& proc;

    struct PanelLnF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override;
        void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
        void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
        juce::Font getLabelFont (juce::Label&) override;
        juce::Font getComboBoxFont (juce::ComboBox&) override;
    };

    // Click the name/version in the top left. "Check for update" asks GitHub for tags.
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

    // "<  text  >" bar: the 20px ends step, the middle opens a menu. Shared with K1X.
    struct PatchBar : public juce::Component,
                      public juce::SettableTooltipClient
    {
        void paint (juce::Graphics& g) override;
        void mouseUp (const juce::MouseEvent& e) override;
        void setText (const juce::String& t);

        juce::String text;
        std::function<void (int)> onStep;
        std::function<void()> onOpen;
    };

    // Level fader (left) + output spectrum (right). Both flash yellow on clip.
    struct VolumeStrip : public juce::Component,
                         public juce::SettableTooltipClient
    {
        explicit VolumeStrip (F4RTAudioProcessorEditor& ed);
        ~VolumeStrip() override;
        void resized() override;
        void paint (juce::Graphics& g) override;
        void refresh();
        juce::Rectangle<float> faderArea() const;
        juce::Rectangle<float> scopeArea() const;

        struct LnF : public juce::LookAndFeel_V4
        {
            void drawLinearSlider (juce::Graphics&, int, int, int, int, float pos,
                                   float, float, juce::Slider::SliderStyle, juce::Slider&) override;
        };

        F4RTAudioProcessorEditor& editor;
        LnF lnf;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAtt;
        bool clipped = false;
        juce::uint32 clipUntil = 0;
    };

    struct Knob : public juce::Component,
                  public juce::SettableTooltipClient
    {
        Knob (juce::AudioProcessorValueTreeState& st, const juce::String& id,
              const juce::String& name, const juce::String& tip);
        void resized() override;
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    struct Combo : public juce::Component,
                   public juce::SettableTooltipClient
    {
        Combo (juce::AudioProcessorValueTreeState& st, const juce::String& id,
               const juce::String& name, const juce::String& tip);
        void resized() override;
        juce::Label label;
        juce::ComboBox box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
    };

    // On/off switch for a bool parameter (matrix row on, FX on, LFO sync).
    struct Toggle : public juce::Component,
                    public juce::SettableTooltipClient
    {
        Toggle (juce::AudioProcessorValueTreeState& st, const juce::String& id,
                const juce::String& name, const juce::String& tip);
        void resized() override;
        juce::ToggleButton btn;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };

    struct Viz : public juce::Component,
                 public juce::SettableTooltipClient
    {
        explicit Viz (F4RTAudioProcessorEditor& e) : ed (e) {}
        void paint (juce::Graphics&) override;
        F4RTAudioProcessorEditor& ed;
        int mode = 0;
    };

    struct BrowserModel : public juce::ListBoxModel
    {
        F4RTAudioProcessorEditor* ed = nullptr;
        juce::StringArray rows;
        int getNumRows() override { return rows.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override;
        void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    };

    PanelLnF panelLnF;
    Content content { *this };  // declared before its children so it is destroyed after them
    int baseW = 0, baseH = 0;

    // Owners. Pages only hold pointers into these.
    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<Combo> combos;
    juce::OwnedArray<Toggle> toggles;
    // Per page: grid cells (nullptr = empty cell) and the button row above them.
    juce::Array<juce::Component*> pageItems[kPageCount];
    juce::Array<juce::Component*> pageButtons[kPageCount];
    int pageCols[kPageCount] {};

    PatchBar pageBar, patchBar, bankBar;
    VolumeStrip volumeStrip { *this };

    juce::ToggleButton playBtn { "Play" };
    juce::ToggleButton randAll { "All" };
    juce::ToggleButton randSafe { "Safe" };
    juce::ToggleButton randFx { "FX" };
    juce::ToggleButton randMut { "Mutate" };
    juce::ToggleButton saveBtn { "Save As" };
    juce::ToggleButton loadBtn { "Import" };
    juce::ToggleButton favBtn { "Fav" };
    juce::ToggleButton wtBtn { "Import WT" };
    juce::ToggleButton recipeBtn { "Apply FM" };
    juce::TextEditor search;
    juce::ListBox browser;
    BrowserModel browserModel;
    juce::ComboBox vizMode;
    Viz viz { *this };
    AboutBox aboutBox;
    int editorPage = 0;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Image logo;
    juce::TooltipWindow tip { this, 400 };  // hover help; 400 ms so it shows before you look away

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (F4RTAudioProcessorEditor)
};
