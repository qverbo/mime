#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace mime::ui
{

namespace colours
{
    const juce::Colour bg       { 0xff0b0b0f };
    const juce::Colour panel    { 0xff14141b };
    const juce::Colour panel2   { 0xff1c1c25 };
    const juce::Colour line     { 0xff2a2a36 };
    const juce::Colour text     { 0xffeeedf4 };
    const juce::Colour dim      { 0xff8b8a9b };
    const juce::Colour accent   { 0xffe8364f };   // crimson
    const juce::Colour ghost    { 0xffc9c3ff };   // pale lilac
}

inline juce::Font font (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

class MimeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MimeLookAndFeel()
    {
        using namespace colours;
        setColour (juce::ResizableWindow::backgroundColourId, bg);
        setColour (juce::Slider::textBoxTextColourId, text);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Label::textColourId, text);
        setColour (juce::ComboBox::backgroundColourId, panel2);
        setColour (juce::ComboBox::outlineColourId, line);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::arrowColourId, dim);
        setColour (juce::PopupMenu::backgroundColourId, panel2);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.35f));
        setColour (juce::TextButton::buttonColourId, panel2);
        setColour (juce::TextButton::buttonOnColourId, accent);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::ToggleButton::textColourId, text);
        setColour (juce::ToggleButton::tickColourId, accent);
        setColour (juce::ToggleButton::tickDisabledColourId, dim);
        setColour (juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::textColourId, text);
        setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::ScrollBar::thumbColourId, line);
        setColour (juce::TooltipWindow::backgroundColourId, panel2);
        setColour (juce::TooltipWindow::textColourId, text);
        setColour (juce::TooltipWindow::outlineColourId, line);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        using namespace colours;
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
        const float rad = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const float thick = juce::jmax (2.5f, rad * 0.14f);
        const float ang = startAngle + pos * (endAngle - startAngle);

        // body
        g.setColour (panel2);
        g.fillEllipse (juce::Rectangle<float> (rad * 2 - thick * 2.4f, rad * 2 - thick * 2.4f).withCentre (c));

        // track
        juce::Path track;
        track.addCentredArc (c.x, c.y, rad - thick * 0.5f, rad - thick * 0.5f, 0, startAngle, endAngle, true);
        g.setColour (line);
        g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // value (bipolar knobs fill from the top)
        const bool bipolar = s.getMinimum() < 0.0;
        const float from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;
        juce::Path val;
        val.addCentredArc (c.x, c.y, rad - thick * 0.5f, rad - thick * 0.5f, 0, juce::jmin (from, ang), juce::jmax (from, ang), true);
        const auto col = s.findColour (juce::Slider::rotarySliderFillColourId);
        g.setColour (col);
        g.strokePath (val, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // pointer
        const float pr = rad - thick * 2.4f;
        juce::Point<float> tip (c.x + std::sin (ang) * pr * 0.82f, c.y - std::cos (ang) * pr * 0.82f);
        g.setColour (text);
        g.drawLine ({ c, tip }, 2.0f);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bgCol,
                               bool over, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        auto col = b.getToggleState() ? b.findColour (juce::TextButton::buttonOnColourId) : bgCol;
        if (down) col = col.brighter (0.15f);
        else if (over) col = col.brighter (0.07f);
        g.setColour (col);
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (colours::line.brighter (over ? 0.3f : 0.0f));
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return font (juce::jmin (15.0f, h * 0.42f), true); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (14.0f); }
    juce::Font getPopupMenuFont() override { return font (14.0f); }
    juce::Font getLabelFont (juce::Label& l) override { return l.getFont(); }
};

} // namespace mime::ui
