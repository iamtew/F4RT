#pragma once

#include "dsp/Analyser.h"
#include "dsp/FxRack.h"
#include "dsp/SynthEngine.h"
#include "parameters/ParameterLayout.h"

#include <JuceHeader.h>
#include <atomic>

class F4RTAudioProcessor : public juce::AudioProcessor
{
public:
    F4RTAudioProcessor();
    ~F4RTAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "F4RT"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    bool supportsMPE() const override { return true; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    f4rt::Params currentParams() const;
    f4rt::Matrix currentMatrix() const;
    void writeParams (const f4rt::Params& p);
    void randomize (int kind);
    void loadFactory (int index);
    bool importWavetable (const juce::File& file);

    juce::File patchesDir() const;
    juce::File favFile() const;
    juce::StringArray patchNames() const;
    juce::String patchName() const;
    void initPatch();
    bool loadPatchByName (const juce::String& name);
    bool savePatch();
    bool savePatchAs (const juce::String& name);
    bool exportPatch (const juce::File& dest);
    bool importPatch (const juce::File& src);
    juce::StringArray favoriteNames() const;
    void toggleFavorite (const juce::String& name);
    bool isFavorite (const juce::String& name) const;

    // 0..kFactoryCount-1 while a factory patch is loaded, -1 for Init and user patches.
    // Lives in the state tree so it survives session reloads.
    int factoryIndex() const;

    f4rt::Analyser analyser;
    float envMon[6] {};
    juce::String searchText;
    float uiScale = 0.9f;
    std::atomic<bool> clipFlag { false };  // set by the audio thread, consumed by the volume strip

    void requestNote() { pendingNote.store (true, std::memory_order_relaxed); }

private:
    float raw (const char* id) const;
    int choice (const char* id) const;
    float hostBpm() const;
    void setF (const char* id, float v);
    void setChoice (const char* id, int v);
    void setB (const char* id, bool v);

    f4rt::SynthEngine engine;
    f4rt::FxRack fx;
    std::atomic<bool> pendingNote { false };
    uint32_t mutateSeed = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (F4RTAudioProcessor)
};
