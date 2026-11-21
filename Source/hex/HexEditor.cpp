#include "HexEditor.h"
#include "BinaryData.h"

using namespace hexui;
using hex::Fx;

// ============================================================ fonts / look

Fonts& Fonts::get()
{
    static Fonts f = []
    {
        Fonts x;
        x.goth = juce::Typeface::createSystemTypefaceFor (BinaryData::GrenzeGotischSemiBold_ttf, BinaryData::GrenzeGotischSemiBold_ttfSize);
        x.wide = juce::Typeface::createSystemTypefaceFor (BinaryData::MichromaRegular_ttf, BinaryData::MichromaRegular_ttfSize);
        x.mono = juce::Typeface::createSystemTypefaceFor (BinaryData::JetBrainsMonoMedium_ttf, BinaryData::JetBrainsMonoMedium_ttfSize);
        return x;
    }();
    return f;
}

LookAndFeel::LookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, col::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TooltipWindow::backgroundColourId, col::raised);
    setColour (juce::TooltipWindow::textColourId, col::text);
    setColour (juce::TooltipWindow::outlineColourId, col::line);
    setColour (juce::PopupMenu::backgroundColourId, col::raised);
    setColour (juce::TextButton::textColourOffId, col::text);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 4.0f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    // tick marks
    for (int i = 0; i <= 20; ++i)
    {
        const float t = (float) i / 20.0f;
        const float a = a0 + t * (a1 - a0);
        const bool lit = t <= pos + 0.001f;
        const float r0 = r - (i % 5 == 0 ? 6.0f : 4.0f);
        g.setColour (lit ? col::accent.withAlpha (0.55f + 0.45f * t) : col::line.brighter (0.25f));
        g.drawLine (c.x + std::sin (a) * r0, c.y - std::cos (a) * r0, c.x + std::sin (a) * r, c.y - std::cos (a) * r, 1.3f);
    }

    // body
    const float br = r - 10.0f;
    juce::ColourGradient body (col::raised.brighter (0.15f), c.x, c.y - br, col::bg, c.x, c.y + br, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (col::line.brighter (0.2f));
    g.drawEllipse (c.x - br, c.y - br, br * 2, br * 2, 1.0f);

    // indicator dot
    const float dr = br - 5.0f;
    const juce::Point<float> dot (c.x + std::sin (ang) * dr, c.y - std::cos (ang) * dr);
    g.setColour (col::hot.withAlpha (s.isMouseOverOrDragging() ? 1.0f : 0.85f));
    g.fillEllipse (dot.x - 2.5f, dot.y - 2.5f, 5.0f, 5.0f);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const bool prime = b.getProperties().getWithDefault ("prime", false);
    juce::Colour fill = on ? col::accent.withAlpha (0.18f) : col::panel;
    if (prime) fill = col::accent.withAlpha (down ? 0.55f : over ? 0.45f : 0.35f);
    else if (over) fill = fill.brighter (0.06f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (on || prime ? col::accent : col::line.brighter (over ? 0.35f : 0.1f));
    g.drawRoundedRectangle (r, 3.0f, 1.0f);
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool on = b.getToggleState() || (bool) b.getProperties().getWithDefault ("prime", false);
    if (b.getProperties().contains ("swatch"))
    {
        const auto argb = (juce::uint32) (juce::int64) b.getProperties()["swatch"];
        auto sw = juce::Rectangle<float> (10.0f, b.getHeight() * 0.5f - 4.0f, 8.0f, 8.0f);
        if (argb == 0) { g.setColour (col::dim); g.drawLine (sw.getX(), sw.getBottom(), sw.getRight(), sw.getY(), 1.2f); }
        else if (argb == 0xff2b2236u) { g.setColour (col::dim); g.drawRect (sw, 1.0f); }
        else { g.setColour (juce::Colour (argb)); g.fillRect (sw); }
    }
    g.setColour (on ? col::text : col::dim.brighter (0.3f));
    g.setFont (Fonts::get().label (juce::jmin (10.0f, b.getHeight() * 0.36f)));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

juce::Label* LookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (Fonts::get().num (12.0f));
    l->setColour (juce::Label::textColourId, col::text.withAlpha (0.9f));
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setJustificationType (juce::Justification::centred);
    return l;
}

// ============================================================ ring sequencer

namespace hexui
{
class Ring : public juce::Component
{
public:
    Ring (HexProcessor& p, HexEditor& e) : proc (p), ed (e)
    {
        sigil = juce::ImageCache::getFromMemory (BinaryData::hex_sigil_png, BinaryData::hex_sigil_pngSize);
    }

    void tick()
    {
        const bool running = proc.uiSync.load() != 0;
        const int fx = proc.currentFx();
        const float target = (running && fx != 0) ? 1.0f : 0.0f;
        energy += (target - energy) * (target > energy ? 0.5f : 0.08f);
        spin += running ? 0.0035f + 0.02f * energy : 0.0006f;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        const float rOut = juce::jmin (getWidth(), getHeight()) * 0.5f - 18.0f;
        const float rIn = rOut * 0.78f;
        const int cur = proc.currentStep();
        const int curFx = proc.currentFx();
        auto& F = Fonts::get();

        // aura
        const auto auraCol = curFx != 0 ? juce::Colour (hex::fxInfo ((Fx) curFx).colour) : col::accent;
        juce::ColourGradient aura (auraCol.withAlpha (0.10f + 0.22f * energy), c.x, c.y,
                                   juce::Colours::transparentBlack, c.x + rOut * 1.05f, c.y, true);
        g.setGradientFill (aura);
        g.fillEllipse (juce::Rectangle<float> (rOut * 2.1f, rOut * 2.1f).withCentre (c));

        // sigil
        if (sigil.isValid())
        {
            const float d = rIn * 2.0f - 22.0f;
            const float sc = d / (float) sigil.getWidth();
            auto tr = juce::AffineTransform::translation (-sigil.getWidth() * 0.5f, -sigil.getHeight() * 0.5f)
                          .scaled (sc * (1.0f + 0.025f * energy))
                          .rotated (spin)
                          .translated (c.x, c.y);
            g.setOpacity (0.55f + 0.45f * energy);
            g.drawImageTransformed (sigil, tr);
            g.setOpacity (1.0f);
        }

        // steps
        const float seg = juce::MathConstants<float>::twoPi / hex::numSteps;
        const float gap = 0.022f;
        for (int i = 0; i < hex::numSteps; ++i)
        {
            const int fx = proc.getStep (i);
            const auto& info = hex::fxInfo ((Fx) fx);
            juce::Path p;
            p.addPieSegment (juce::Rectangle<float> (rOut * 2, rOut * 2).withCentre (c),
                             i * seg + gap, (i + 1) * seg - gap, rIn / rOut);
            const bool isCur = i == cur;
            const bool hov = i == hover;
            juce::Colour fill = fx == 0 ? col::panel : juce::Colour (info.colour).withAlpha (0.82f);
            if (fx == (int) Fx::Gate) fill = juce::Colour (0xff050407);
            if (isCur) fill = fx == 0 ? col::raised.brighter (0.25f) : fill.brighter (0.35f).withAlpha (1.0f);
            g.setColour (fill);
            g.fillPath (p);
            g.setColour (isCur ? col::hot : hov ? col::accent : (fx == 0 ? col::line : juce::Colour (info.colour).brighter (0.2f).withAlpha (0.6f)));
            g.strokePath (p, juce::PathStrokeType (isCur ? 1.6f : 1.0f));

            // label
            const float am = (i + 0.5f) * seg;
            const float rm = (rIn + rOut) * 0.5f;
            const juce::Point<float> lp (c.x + std::sin (am) * rm, c.y - std::cos (am) * rm);
            if (fx != 0)
            {
                const bool light = fx == (int) Fx::OctUp || fx == (int) Fx::Scatter;
                g.setColour (fx == (int) Fx::Gate ? col::dim : (light ? col::bg : col::text));
                g.setFont (F.label (10.0f, 0.05f));
                g.drawText (info.code, juce::Rectangle<float> (34, 14).withCentre (lp), juce::Justification::centred);
            }

            // beat markers
            if (i % 4 == 0)
            {
                const juce::Point<float> mp (c.x + std::sin (i * seg) * (rOut + 9), c.y - std::cos (i * seg) * (rOut + 9));
                g.setColour (col::dim);
                g.setFont (F.num (10.0f));
                g.drawText (juce::String (i / 4 + 1), juce::Rectangle<float> (16, 12).withCentre (mp), juce::Justification::centred);
            }
        }

        // playhead
        if (cur >= 0)
        {
            const float a = (cur + proc.stepPhase()) * seg;
            juce::Path ph;
            ph.addCentredArc (c.x, c.y, rOut + 3.0f, rOut + 3.0f, 0.0f, cur * seg + gap, a, true);
            g.setColour (col::hot);
            g.strokePath (ph, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // inner rim
        g.setColour (col::line);
        g.drawEllipse (juce::Rectangle<float> (rIn * 2 - 8, rIn * 2 - 8).withCentre (c), 1.0f);
    }

    int stepAt (juce::Point<float> p) const
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        const float rOut = juce::jmin (getWidth(), getHeight()) * 0.5f - 18.0f;
        const float rIn = rOut * 0.78f;
        const float d = p.getDistanceFrom (c);
        if (d < rIn - 4 || d > rOut + 6) return -1;
        float a = std::atan2 (p.x - c.x, -(p.y - c.y));
        if (a < 0) a += juce::MathConstants<float>::twoPi;
        return juce::jlimit (0, hex::numSteps - 1, (int) (a / (juce::MathConstants<float>::twoPi / hex::numSteps)));
    }

    void mouseMove (const juce::MouseEvent& e) override { hover = stepAt (e.position); }
    void mouseExit (const juce::MouseEvent&) override { hover = -1; }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int s = stepAt (e.position);
        if (s < 0) return;
        if (e.mods.isPopupMenu()) { paintValue = 0; }
        else paintValue = proc.getStep (s) == ed.brush ? 0 : ed.brush;
        proc.setStep (s, paintValue);
        lastPainted = s;
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        const int s = stepAt (e.position);
        if (s >= 0 && s != lastPainted) { proc.setStep (s, paintValue); lastPainted = s; }
    }

private:
    HexProcessor& proc;
    HexEditor& ed;
    juce::Image sigil;
    float spin = 0.0f, energy = 0.0f;
    int hover = -1, paintValue = 0, lastPainted = -1;
};

// ============================================================ pattern slot

class SlotView : public juce::Component, public juce::SettableTooltipClient
{
public:
    SlotView (HexProcessor& p, int i) : proc (p), index (i) {}

    void paint (juce::Graphics& g) override
    {
        auto& F = Fonts::get();
        const auto& s = proc.getSlot (index);
        const bool sel = proc.getSelectedSlot() == index;
        auto r = getLocalBounds().toFloat().reduced (0.5f);

        g.setColour (sel ? col::raised : col::panel);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (sel ? col::accent : (hover ? col::line.brighter (0.4f) : col::line));
        g.drawRoundedRectangle (r, 4.0f, 1.0f);

        auto inner = r.reduced (10.0f, 8.0f);
        auto top = inner.removeFromTop (14.0f);
        lockBox = top.removeFromRight (14.0f);
        drawLock (g, lockBox, s.locked);
        g.setColour (col::dim);
        g.setFont (F.num (10.0f));
        g.drawText ("0" + juce::String (index + 1) + (sel && proc.isEdited() ? "  EDITED" : ""), top, juce::Justification::centredLeft);

        auto nameRow = inner.removeFromBottom (22.0f);
        g.setColour (sel ? col::text : col::text.withAlpha (0.72f));
        g.setFont (F.gothic (20.0f));
        auto nm = s.name.toLowerCase();
        juce::String pretty;
        for (auto w : juce::StringArray::fromTokens (nm, " ", ""))
            pretty << (w.length() > 0 && ! w.containsOnly ("ivxl") ? w.substring (0, 1).toUpperCase() + w.substring (1) : w.toUpperCase()) << " ";
        g.drawText (pretty.trim(), nameRow, juce::Justification::centred);

        // mini ring
        const float d = juce::jmin (inner.getWidth(), inner.getHeight()) - 6.0f;
        const auto c = inner.getCentre();
        const float ro = d * 0.5f, ri = ro * 0.6f;
        const float seg = juce::MathConstants<float>::twoPi / hex::numSteps;
        for (int i = 0; i < hex::numSteps; ++i)
        {
            const int fx = s.steps[(size_t) i];
            juce::Path p;
            p.addPieSegment (juce::Rectangle<float> (ro * 2, ro * 2).withCentre (c), i * seg + 0.035f, (i + 1) * seg - 0.035f, ri / ro);
            g.setColour (fx == 0 ? col::line : juce::Colour (hex::fxInfo ((Fx) fx).colour));
            g.fillPath (p);
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! getLocalBounds().contains (e.getPosition())) return;
        if (lockBox.expanded (6.0f).contains (e.position)) proc.toggleLock (index);
        else proc.loadSlot (index);
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hover = false; repaint(); }

private:
    static void drawLock (juce::Graphics& g, juce::Rectangle<float> a, bool locked)
    {
        auto body = a.withTrimmedTop (a.getHeight() * 0.45f).reduced (1.5f, 0.0f);
        juce::Path sh;
        const float x0 = body.getX() + body.getWidth() * 0.25f, x1 = body.getRight() - body.getWidth() * 0.25f;
        sh.startNewSubPath (x0, body.getY());
        sh.lineTo (x0, a.getY() + 3.0f);
        sh.quadraticTo ((x0 + x1) * 0.5f, a.getY() - 2.0f, x1, a.getY() + 3.0f);
        sh.lineTo (x1, locked ? body.getY() : body.getY() - 3.0f);
        g.setColour (locked ? col::hot : col::dim);
        g.strokePath (sh, juce::PathStrokeType (1.4f));
        if (locked) g.fillRect (body); else g.drawRect (body, 1.2f);
    }

    HexProcessor& proc;
    int index;
    bool hover = false;
    juce::Rectangle<float> lockBox;
};
} // namespace hexui

// ============================================================ editor

namespace
{
    constexpr int W = 1000, H = 664;
    const char* fxTips[] = {
        "Erase: step plays through clean.",
        "Repeats the start of the step. Classic vocal stutter.",
        "Fast stutter that speeds up and rises in pitch.",
        "Plays the previous step backwards.",
        "Slows down to a stop like pulling the plug on a turntable.",
        "Drops the step an octave, slowed and pitched down.",
        "Jumps the step an octave up. Chipmunk/hyperpop.",
        "Mutes the step. A hard chop.",
        "Swaps in a slice from a few steps earlier.",
    };
}

HexEditor::HexEditor (HexProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);

    ring = std::make_unique<Ring> (proc, *this);
    addAndMakeVisible (*ring);

    // brush palette
    for (int i = 0; i < (int) Fx::NumFx; ++i)
    {
        auto b = std::make_unique<juce::TextButton> (i == 0 ? "ERASE" : hex::fxInfo ((Fx) i).name);
        b->setTooltip (fxTips[i]);
        b->onClick = [this, i] { brush = i; for (int k = 0; k < (int) brushButtons.size(); ++k) brushButtons[(size_t) k]->setToggleState (k == brush, juce::dontSendNotification); };
        b->setToggleState (i == brush, juce::dontSendNotification);
        b->getProperties().set ("swatch", (juce::int64) (i == 0 ? 0 : hex::fxInfo ((Fx) i).colour));
        addAndMakeVisible (*b);
        brushButtons.push_back (std::move (b));
    }

    // style selector
    const auto styles = hex::styleNames();
    const char* styleTips[] = { "Stutters, rolls and octave jumps. Hyperpop energy.",
                                "Reverses, tape stops and pitched-down drags. Witch house.",
                                "Gates and hard stutters on the groove. Rage.",
                                "Clean bar, chaos in the last few steps. Classic fill.",
                                "Anything goes." };
    for (int i = 0; i < styles.size(); ++i)
    {
        auto b = std::make_unique<juce::TextButton> (styles[i]);
        b->setTooltip (styleTips[i]);
        b->onClick = [this, i] { if (styleAtt) styleAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (*b);
        styleButtons.push_back (std::move (b));
    }
    styleAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter ("style"), [this] (float v)
    {
        for (int k = 0; k < (int) styleButtons.size(); ++k) styleButtons[(size_t) k]->setToggleState (k == juce::roundToInt (v), juce::dontSendNotification);
    });
    styleAtt->sendInitialUpdate();

    regenButton.getProperties().set ("prime", true);
    regenButton.setTooltip ("Roll six new patterns in the selected style. Locked slots stay.");
    regenButton.onClick = [this] { proc.regenerate(); };
    addAndMakeVisible (regenButton);
    clearButton.setTooltip ("Clear the ring.");
    clearButton.onClick = [this] { proc.clearPattern(); };
    addAndMakeVisible (clearButton);

    for (int i = 0; i < HexProcessor::numSlots; ++i)
    {
        auto v = std::make_unique<SlotView> (proc, i);
        v->setTooltip ("Click to load. Lock to keep it through REGEN.");
        addAndMakeVisible (*v);
        slotViews.push_back (std::move (v));
    }

    auto addKnob = [this] (const char* id, const char* name, const char* tip)
    {
        auto k = std::make_unique<Knob>();
        k->name = name;
        k->slider.setLookAndFeel (&lnf);
        k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k->slider.setColour (juce::Slider::textBoxTextColourId, col::text);
        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
        k->slider.setTooltip (tip);
        addAndMakeVisible (k->slider);
        k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
        knobs.push_back (std::move (k));
    };
    addKnob ("mix",     "MIX",     "Blend between the dry vocal and the glitched steps.");
    addKnob ("chaos",   "CHAOS",   "Random surprise glitches on top of the pattern, different every bar.");
    addKnob ("density", "DENSITY", "How busy REGEN makes new patterns.");
    addKnob ("grid",    "GRID",    "Step size. 1/16 = one bar around the ring. 1/8 = two bars. 1/32 = half a bar.");
    addKnob ("repeats", "REPEATS", "How many times a stutter repeats inside one step.");
    addKnob ("smooth",  "SMOOTH",  "Crossfade length between steps. Low = sharp and clicky, high = smooth.");
    addKnob ("swing",   "SWING",   "Pushes every second step late for a swung groove.");
    addKnob ("output",  "OUTPUT",  "Output level.");

    proc.addChangeListener (this);
    setSize (W, H);
    startTimerHz (60);
}

HexEditor::~HexEditor()
{
    proc.removeChangeListener (this);
    for (auto& k : knobs) k->slider.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void HexEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    for (auto& v : slotViews) v->repaint();
    ring->repaint();
}

void HexEditor::timerCallback()
{
    ring->tick();
    repaint (header);
}

void HexEditor::paint (juce::Graphics& g)
{
    auto& F = Fonts::get();
    g.fillAll (col::bg);

    // faint vignette behind the ring
    const auto rc = ringArea.toFloat().getCentre();
    juce::ColourGradient v (col::accent.withAlpha (0.07f), rc.x, rc.y, juce::Colours::transparentBlack, rc.x + 520.0f, rc.y, true);
    g.setGradientFill (v);
    g.fillRect (getLocalBounds());

    // header
    auto h = header;
    g.setColour (col::text);
    g.setFont (F.gothic (44.0f));
    g.drawText ("Hex Gate", h.removeFromLeft (190), juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (F.label (9.0f, 0.35f));
    g.drawText ("VOCAL GLITCH ENGINE", h.removeFromLeft (260).withTrimmedTop (10), juce::Justification::centredLeft);

    const int sync = proc.uiSync.load();
    auto rt = h.removeFromRight (300);
    g.setColour (col::dim);
    g.setFont (F.label (8.5f, 0.25f));
    g.drawText ("BPM", rt.removeFromRight (34).withTrimmedTop (9), juce::Justification::centredRight);
    g.setFont (F.num (20.0f));
    g.setColour (col::text);
    g.drawText (juce::String (proc.uiBpm.load(), 1), rt.removeFromRight (70), juce::Justification::centredRight);
    rt.removeFromRight (26);
    const juce::String syncTxt = sync == 1 ? "HOST SYNC" : sync == 2 ? "FREE RUN" : "STOPPED";
    g.setColour (col::dim);
    g.setFont (F.label (8.5f, 0.25f));
    auto sr = rt.removeFromRight (100).withTrimmedTop (9);
    g.drawText (syncTxt, sr, juce::Justification::centredRight);
    const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), syncTxt);
    g.setColour (sync == 0 ? col::line.brighter (0.4f) : col::accent);
    g.fillEllipse (juce::Rectangle<float> (6, 6).withCentre ({ (float) sr.getRight() - tw - 12.0f, (float) sr.getCentreY() }));

    g.setColour (col::line);
    g.drawHorizontalLine (header.getBottom() + 4, (float) header.getX(), (float) header.getRight());

    // section headings
    auto heading = [&] (juce::Rectangle<int> r, const juce::String& t)
    {
        g.setColour (col::dim);
        g.setFont (F.label (8.5f, 0.3f));
        g.drawText (t, r.removeFromTop (14), juce::Justification::centredLeft);
    };
    heading (brushArea, "BRUSH");
    heading (bankArea, "PATTERN BANK");

    // knob strip
    g.setColour (col::line);
    g.drawHorizontalLine (knobArea.getY() - 8, (float) knobArea.getX(), (float) knobArea.getRight());
    for (auto& k : knobs)
    {
        auto b = k->slider.getBounds();
        g.setColour (col::dim);
        g.setFont (F.label (8.5f, 0.25f));
        g.drawText (k->name, b.withHeight (14).translated (0, -14), juce::Justification::centred);
    }

    // footer
    g.setColour (col::dim.withAlpha (0.6f));
    g.setFont (F.label (7.5f, 0.3f));
    g.drawText ("CLICK A STEP TO PAINT   /   DRAG TO PAINT MANY   /   RIGHT-CLICK TO ERASE", ringArea.withTop (ringArea.getBottom() - 12), juce::Justification::centred);
}

void HexEditor::resized()
{
    auto area = getLocalBounds().reduced (26, 18);
    header = area.removeFromTop (52);
    area.removeFromTop (14);

    knobArea = area.removeFromBottom (104);
    area.removeFromBottom (18);

    ringArea = area.removeFromLeft (450);
    ring->setBounds (ringArea.withTrimmedBottom (16));
    area.removeFromLeft (28);

    brushArea = area.removeFromTop (14 + 3 * 30 + 2 * 6);
    {
        auto r = brushArea.withTrimmedTop (18);
        const int bw = (r.getWidth() - 2 * 6) / 3;
        for (int i = 0; i < (int) brushButtons.size(); ++i)
        {
            const int cx = i % 3, cy = i / 3;
            brushButtons[(size_t) i]->setBounds (r.getX() + cx * (bw + 6), r.getY() + cy * 30, bw, 26);
        }
    }
    area.removeFromTop (22);
    bankArea = area;
    {
        auto r = bankArea.withTrimmedTop (18);
        auto row = r.removeFromTop (28);
        auto regen = row.removeFromRight (96);
        row.removeFromRight (6);
        auto clear = row.removeFromRight (70);
        row.removeFromRight (12);
        regenButton.setBounds (regen);
        clearButton.setBounds (clear);
        const int sw = (row.getWidth() - 4 * 4) / 5;
        for (auto& b : styleButtons) { b->setBounds (row.removeFromLeft (sw)); row.removeFromLeft (4); }

        r.removeFromTop (12);
        const int gw = (r.getWidth() - 2 * 8) / 3;
        const int gh = (r.getHeight() - 8) / 2;
        for (int i = 0; i < (int) slotViews.size(); ++i)
            slotViews[(size_t) i]->setBounds (r.getX() + (i % 3) * (gw + 8), r.getY() + (i / 3) * (gh + 8), gw, gh);
    }

    auto k = knobArea.withTrimmedTop (14);
    const int kw = k.getWidth() / (int) knobs.size();
    for (auto& kn : knobs) kn->slider.setBounds (k.removeFromLeft (kw).reduced (6, 0));
}
