#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PossProcessor.h"

namespace possui
{
    namespace col
    {
        const juce::Colour bg     { 0xff070708 };
        const juce::Colour panel  { 0xff0e0e11 };
        const juce::Colour raised { 0xff16161b };
        const juce::Colour line   { 0xff24242b };
        const juce::Colour bone   { 0xffece8ef };
        const juce::Colour dim    { 0xff7c7a86 };
        const juce::Colour silver { 0xffc6c9d3 };
        const juce::Colour ice    { 0xffdfe6ff };
    }

    struct Fonts
    {
        juce::Typeface::Ptr goth, wide, mono;
        static Fonts& get();
        juce::Font gothic (float h) const { return juce::Font (juce::FontOptions (goth).withHeight (h)); }
        juce::Font label  (float h, float track = 0.12f) const { return juce::Font (juce::FontOptions (wide).withHeight (h).withKerningFactor (track)); }
        juce::Font num    (float h) const { return juce::Font (juce::FontOptions (mono).withHeight (h)); }
    };

    class LookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        LookAndFeel();
        void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
        void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
        void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
        juce::Label* createSliderTextBox (juce::Slider&) override;
    };

    class Art;
    class Card;
}

class PossEditor : public juce::AudioProcessorEditor,
                   public juce::FileDragAndDropTarget,
                   private juce::ChangeListener,
                   private juce::Timer
{
public:
    explicit PossEditor (PossProcessor&);
    ~PossEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void fileDragEnter (const juce::StringArray&, int, int) override { dragging = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragging = false; repaint(); }
    void filesDropped (const juce::StringArray&, int, int) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    struct Knob { juce::Slider slider; juce::String name; std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att; };

    PossProcessor& proc;
    possui::LookAndFeel lnf;
    juce::TooltipWindow tips { this, 600 };
    std::unique_ptr<possui::Art> art;
    std::vector<std::unique_ptr<possui::Card>> cards;
    std::vector<std::unique_ptr<juce::TextButton>> roleButtons;
    juce::TextButton regenButton { "REGEN" }, moreButton { "MORE LIKE THIS" }, loadButton { "LOAD BEAT" };
    std::vector<std::unique_ptr<Knob>> knobs;
    std::unique_ptr<juce::FileChooser> chooser;

    juce::Rectangle<int> header, artArea, descArea, knobArea;
    bool dragging = false;
    int shownRole = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PossEditor)
};
