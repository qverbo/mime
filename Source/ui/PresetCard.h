#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "MimeLookAndFeel.h"
#include "../presets/PresetGenerator.h"

namespace mime::ui
{

class PresetCard : public juce::Component, public juce::SettableTooltipClient
{
public:
    std::function<void()> onSelect, onToggleLock;

    void setPreset (const mime::Preset& p, bool isSelected)
    {
        preset = p;
        selected = isSelected;
        setTooltip (p.isEmpty() ? juce::String() : p.shortDesc);
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using namespace colours;
        auto r = getLocalBounds().toFloat().reduced (1.0f);

        g.setColour (selected ? panel2.brighter (0.05f) : panel);
        g.fillRoundedRectangle (r, 10.0f);

        const auto edge = selected ? accent : (hover ? line.brighter (0.4f) : line);
        g.setColour (edge);
        g.drawRoundedRectangle (r, 10.0f, selected ? 2.0f : 1.0f);

        if (preset.isEmpty()) return;

        auto inner = r.reduced (12.0f, 10.0f);

        // lock
        lockArea = juce::Rectangle<float> (inner.getRight() - 20.0f, inner.getY(), 20.0f, 20.0f);
        drawLock (g, lockArea, preset.locked);

        auto top = inner.removeFromTop (22.0f);
        top.removeFromRight (24.0f);
        g.setColour (text);
        g.setFont (font (16.5f, true));
        g.drawFittedText (preset.name.toUpperCase(), top.toNearestInt(), juce::Justification::centredLeft, 1, 0.8f);

        auto laneRow = inner.removeFromTop (18.0f);
        g.setColour (ghost);
        g.setFont (font (12.5f));
        g.drawFittedText ("lane: " + preset.lane, laneRow.toNearestInt(), juce::Justification::centredLeft, 1, 0.85f);

        inner.removeFromTop (4.0f);
        auto chips = inner.removeFromBottom (18.0f);
        g.setColour (dim);
        g.setFont (font (12.5f));
        g.drawFittedText (preset.shortDesc, inner.toNearestInt(), juce::Justification::topLeft, 4, 0.9f);

        // style chips
        const auto& s = preset.settings;
        juce::StringArray tags { mime::styleNames()[s.style], mime::textureNames()[s.texture],
                                 juce::String (juce::roundToInt (s.mix * 100.0f)) + "% mix" };
        float cx = chips.getX();
        g.setFont (font (11.0f, true));
        for (auto& t : tags)
        {
            const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 12.0f;
            juce::Rectangle<float> chip (cx, chips.getY(), tw, chips.getHeight());
            g.setColour (line);
            g.fillRoundedRectangle (chip, 5.0f);
            g.setColour (text.withAlpha (0.8f));
            g.drawText (t, chip, juce::Justification::centred);
            cx += tw + 5.0f;
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (preset.isEmpty() || ! getLocalBounds().contains (e.getPosition())) return;
        if (lockArea.expanded (6.0f).contains (e.position)) { if (onToggleLock) onToggleLock(); }
        else if (onSelect) onSelect();
    }

    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

private:
    static void drawLock (juce::Graphics& g, juce::Rectangle<float> a, bool locked)
    {
        using namespace colours;
        const auto col = locked ? accent : dim.withAlpha (0.7f);
        auto bodyR = a.withTrimmedTop (a.getHeight() * 0.45f).reduced (2.0f, 0.0f);
        juce::Path shackle;
        const float sx = bodyR.getX() + bodyR.getWidth() * 0.22f, ex = bodyR.getRight() - bodyR.getWidth() * 0.22f;
        const float sy = bodyR.getY();
        const float top = a.getY() + 2.0f;
        shackle.startNewSubPath (sx, sy);
        shackle.lineTo (sx, top + 4.0f);
        shackle.quadraticTo ((sx + ex) * 0.5f, top - 3.0f, ex, top + 4.0f);
        shackle.lineTo (ex, locked ? sy : sy - 4.0f);
        g.setColour (col);
        g.strokePath (shackle, juce::PathStrokeType (2.0f));
        if (locked) g.fillRoundedRectangle (bodyR, 2.5f);
        else        g.drawRoundedRectangle (bodyR, 2.5f, 1.6f);
    }

    mime::Preset preset;
    bool selected = false, hover = false;
    juce::Rectangle<float> lockArea;
};

} // namespace mime::ui
