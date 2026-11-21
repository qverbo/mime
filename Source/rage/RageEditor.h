#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "RageProcessor.h"
#include "../ui/MimeLookAndFeel.h"

class RageEditor : public juce::AudioProcessorEditor,
                   private juce::ChangeListener,
                   private juce::Timer
{
public:
    explicit RageEditor (RageProcessor&);
    ~RageEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override { repaint(); }
    void timerCallback() override;
    void updateCrunchButtons();

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    Knob& addKnob (const juce::String& id, const juce::String& name, const juce::String& tip, juce::Colour col);

    RageProcessor& proc;
    mime::ui::MimeLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 500 };
    juce::Image photo;

    std::vector<std::unique_ptr<juce::TextButton>> crunchButtons;
    std::unique_ptr<juce::ParameterAttachment> crunchAtt;
    int currentCrunch = 0;

    std::vector<std::unique_ptr<Knob>> knobs;
    Knob *kAmount, *kMix, *kSlam, *kIdle, *kAttack, *kHold, *kRelease, *kSens, *kDetect, *kTone, *kLowCut, *kOut;

    juce::ComboBox modeBox;
    juce::Label modeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAtt;
    juce::TextButton tapButton { "TAP" }, learnButton { "LEARN 808" };
    juce::ToggleButton listenToggle { "Hear detector" }, autoGainToggle { "Auto Gain" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> listenAtt, autoGainAtt;

    juce::Rectangle<int> photoArea, titleArea, statusArea, scopeArea, crunchArea, blurbArea, hitGroup, envGroup, detGroup, outGroup;
    float flash = 0.0f, scMeter = 0.0f;
    int lastHits = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RageEditor)
};
