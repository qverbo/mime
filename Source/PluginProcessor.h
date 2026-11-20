#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include "dsp/MimeEngine.h"
#include "analysis/BeatAnalyzer.h"
#include "presets/PresetGenerator.h"

class MimeProcessor : public juce::AudioProcessor,
                      public juce::ChangeBroadcaster,
                      private juce::Timer
{
public:
    MimeProcessor();
    ~MimeProcessor() override;

    // AudioProcessor
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MIME"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- MIME features (message thread) ----
    void startListening();
    void stopListening();               // finishes early if enough was heard
    bool isListening() const { return listening.load(); }
    float listenProgressSeconds() const { return heardSeconds; }
    static constexpr float listenTargetSeconds = 20.0f;
    static constexpr float listenMinimumSeconds = 4.0f;
    float sidechainLevel() const { return scLevel.load(); }

    void analyseFile (const juce::File& file);
    bool isAnalysingFile() const { return analysingFile.load(); }

    void regenerate();
    void moreLikeThis();
    void toggleLock (int index);
    void applyPreset (int index);
    int  getSelected() const { return selected; }

    const mime::BeatProfile& getProfile() const { return profile; }
    const mime::Preset& getPreset (int i) const { return presets.slots[(size_t) i]; }
    juce::String getStatus() const { return status; }

    float getOutputPeak() const { return engine.outputPeak.load(); }
    float getHeat() const { return engine.heat.load(); }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void timerCallback() override;
    mime::Settings readSettings() const;
    void finishListening();
    void setStatus (const juce::String& s) { status = s; sendChangeMessage(); }

    mime::MimeEngine engine;

    // live listening (audio thread -> FIFO -> message thread analyser)
    std::atomic<bool> listening { false };
    std::atomic<float> scLevel { 0.0f };
    juce::AbstractFifo fifo { 1 << 17 };
    std::vector<float> fifoL, fifoR;
    mime::BeatAnalyzer liveAnalyzer;
    float heardSeconds = 0.0f;
    std::vector<float> drainL, drainR;

    // file analysis
    juce::ThreadPool pool { 1 };
    std::atomic<bool> analysingFile { false }, cancelFile { false };

    mime::BeatProfile profile;
    mime::PresetGenerator presets;
    int selected = -1;
    juce::String status;

    JUCE_DECLARE_WEAK_REFERENCEABLE (MimeProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MimeProcessor)
};
