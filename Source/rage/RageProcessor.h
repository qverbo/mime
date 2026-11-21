#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include "RageEngine.h"

class RageProcessor : public juce::AudioProcessor,
                      public juce::ChangeBroadcaster,
                      private juce::Timer
{
public:
    RageProcessor();
    ~RageProcessor() override { stopTimer(); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FILIPINA RAGE"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // UI hooks
    void tapHit() { engine.triggerManualHit(); }
    void learn808();
    void setTrigger (int t);
    int  getTrigger() const { return (int) apvts.getRawParameterValue ("trigger")->load(); }
    bool isLearning() const { return learning; }
    juce::String getStatus() const { return status; }

    float hitLevel() const     { return engine.hitLevel.load(); }
    float sidechainPeak() const { return engine.scPeak.load(); }
    int   hitCount() const     { return engine.hitCount.load(); }

    static constexpr int historySize = 360;
    // scope history written by the audio thread
    std::array<std::atomic<float>, historySize> histLevel {}, histDetect {};
    std::atomic<float> histThreshold { 0.5f };
    std::atomic<int> histWrite { 0 };

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void timerCallback() override;
    rage::RageSettings readSettings() const;
    void setStatus (const juce::String& s) { status = s; sendChangeMessage(); }

    rage::RageEngine engine;
    int histAccum = 0, histChunk = 441;
    float histLevelMax = 0, histDetectMax = 0;
    bool learning = false;
    juce::String status;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RageProcessor)
};
