#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "HexProcessor.h"

namespace hexui
{
    namespace col
    {
        const juce::Colour bg      { 0xff08060c };
        const juce::Colour panel   { 0xff100c17 };
        const juce::Colour raised  { 0xff171121 };
        const juce::Colour line    { 0xff261d33 };
        const juce::Colour text    { 0xffe9dcff };
        const juce::Colour dim     { 0xff7a6a96 };
        const juce::Colour accent  { 0xffa66bff };
        const juce::Colour hot     { 0xffd9b8ff };
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
        juce::Font getPopupMenuFont() override { return Fonts::get().label (11.0f, 0.05f); }
    };

    class Ring;
    class SlotView;
}

class HexEditor : public juce::AudioProcessorEditor,
                  private juce::ChangeListener,
                  private juce::Timer
{
public:
    explicit HexEditor (HexProcessor&);
    ~HexEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    int brush = (int) hex::Fx::Stutter;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::String name;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    HexProcessor& proc;
    hexui::LookAndFeel lnf;
    juce::TooltipWindow tips { this, 600 };

    std::unique_ptr<hexui::Ring> ring;
    std::vector<std::unique_ptr<hexui::SlotView>> slotViews;
    std::vector<std::unique_ptr<juce::TextButton>> brushButtons, styleButtons;
    std::unique_ptr<juce::ParameterAttachment> styleAtt;
    juce::TextButton regenButton { "REGEN" }, clearButton { "CLEAR" };
    std::vector<std::unique_ptr<Knob>> knobs;

    juce::Rectangle<int> header, ringArea, brushArea, bankArea, knobArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HexEditor)
};
