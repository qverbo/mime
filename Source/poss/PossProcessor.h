#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_events/juce_events.h>
#include "PossPresets.h"

class PossProcessor : public juce::AudioProcessor,
                      public juce::ChangeBroadcaster,
                      private juce::AudioProcessorValueTreeState::Listener
{
public:
    PossProcessor();
    ~PossProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "POSSESSION"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // presets
    const poss::Preset& getPreset (int i) const { return bank.slots[(size_t) i]; }
    int  getSelected() const { return selected; }
    void applyPreset (int i);
    void regenerate();
    void moreLikeThis();
    void toggleLock (int i) { bank.slots[(size_t) i].locked = ! bank.slots[(size_t) i].locked; sendChangeMessage(); }
    void setRole (int role);
    int  getRole() const { return (int) apvts.getRawParameterValue ("role")->load(); }

    // beat
    void analyseFile (const juce::File&);
    bool isAnalysing() const { return analysing.load(); }
    const mime::BeatProfile& getBeat() const { return beat; }

    float leadActivity() const { return engine.leadActivity.load(); }
    bool  leadPresent() const  { return engine.leadPresent.load(); }
    float intensity() const
    {
        auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
        return juce::jlimit (0.0f, 1.0f, v ("mix") * (0.3f * (v ("chorus") + v ("flange") + v ("phase") + v ("wobble") + v ("dirt") + v ("lofi"))
                                                      + 0.7f * v ("ghost") + 0.3f * v ("shimmer") + 0.25f * v ("width")));
    }

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

private:
    void parameterChanged (const juce::String& id, float v) override;
    poss::PossSettings readSettings() const;

    poss::PossEngine engine;
    poss::PresetBank bank;
    mime::BeatProfile beat;
    int selected = -1;
    bool applying = false;
    std::atomic<int> lastRole { 0 };
    std::atomic<bool> restoring { false };

    juce::ThreadPool pool { 1 };
    std::atomic<bool> analysing { false }, cancel { false };

    JUCE_DECLARE_WEAK_REFERENCEABLE (PossProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PossProcessor)
};
