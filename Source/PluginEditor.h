#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ui/MimeLookAndFeel.h"
#include "ui/PresetCard.h"

class MimeEditor : public juce::AudioProcessorEditor,
                   public juce::FileDragAndDropTarget,
                   private juce::ChangeListener,
                   private juce::Timer
{
public:
    explicit MimeEditor (MimeProcessor&);
    ~MimeEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragging = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragging = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void refresh();

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    void addKnob (const juce::String& id, const juce::String& name, const juce::String& tip, juce::Colour col);

    MimeProcessor& proc;
    mime::ui::MimeLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 500 };

    juce::TextButton listenButton { "LISTEN" }, loadButton { "LOAD BEAT FILE" };
    juce::TextButton regenButton { "REGENERATE" }, moreButton { "MORE LIKE THIS" };
    std::array<mime::ui::PresetCard, mime::PresetGenerator::numSlots> cards;
    juce::TextEditor details;

    juce::ComboBox styleBox, textureBox;
    juce::Label styleLabel, textureLabel;
    juce::ToggleButton autoGainButton { "Auto Gain" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> styleAtt, textureAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoGainAtt;

    std::vector<std::unique_ptr<Knob>> knobs;

    std::unique_ptr<juce::FileChooser> chooser;
    juce::Rectangle<int> headerArea, beatArea, statusArea, controlsArea, mimeArea;
    juce::Image mimeGuy;
    float pulse = 0.0f;
    bool dragging = false;
    float scMeter = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MimeEditor)
};
