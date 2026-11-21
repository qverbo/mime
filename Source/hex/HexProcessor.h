#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "HexEngine.h"

class HexProcessor : public juce::AudioProcessor, public juce::ChangeBroadcaster
{
public:
    HexProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "HEX GATE"; }
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

    // pattern editing (message thread)
    int  getStep (int i) const { return engine.getStep (i); }
    void setStep (int i, int fx) { engine.setStep (i, fx); activeEdited = true; sendChangeMessage(); }
    void clearPattern();

    static constexpr int numSlots = 6;
    const hex::Pattern& getSlot (int i) const { return slots[(size_t) i]; }
    void loadSlot (int i);
    void toggleLock (int i) { slots[(size_t) i].locked = ! slots[(size_t) i].locked; sendChangeMessage(); }
    void regenerate();
    int  getSelectedSlot() const { return selectedSlot; }
    bool isEdited() const { return activeEdited; }

    // transport info for the UI
    std::atomic<double> uiBpm { 140.0 };
    std::atomic<int> uiSync { 0 };   // 0 idle, 1 host, 2 free
    int currentStep() const { return engine.currentStep.load(); }
    int currentFx() const   { return engine.currentFx.load(); }
    float stepPhase() const { return engine.stepPhase.load(); }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    hex::HexSettings readSettings() const;

    hex::HexEngine engine;
    std::array<hex::Pattern, numSlots> slots;
    int selectedSlot = 0;
    bool activeEdited = false;
    double freePpq = 0.0;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HexProcessor)
};
