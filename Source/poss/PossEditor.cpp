#include "PossEditor.h"
#include "BinaryData.h"

using namespace possui;

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
    setColour (juce::Slider::textBoxTextColourId, col::bone);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TooltipWindow::backgroundColourId, col::raised);
    setColour (juce::TooltipWindow::textColourId, col::bone);
    setColour (juce::TooltipWindow::outlineColourId, col::line);
}

void LookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 3.0f;
    const auto c = b.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    juce::Path track, val;
    track.addCentredArc (c.x, c.y, r - 1.5f, r - 1.5f, 0, a0, a1, true);
    val.addCentredArc (c.x, c.y, r - 1.5f, r - 1.5f, 0, a0, ang, true);
    g.setColour (col::line.brighter (0.15f));
    g.strokePath (track, juce::PathStrokeType (1.4f));
    g.setColour (col::silver.withAlpha (0.55f + 0.45f * pos));
    g.strokePath (val, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float br = r - 7.0f;
    juce::ColourGradient body (col::raised.brighter (0.12f), c.x, c.y - br, col::bg, c.x, c.y + br, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (col::line.brighter (0.25f));
    g.drawEllipse (c.x - br, c.y - br, br * 2, br * 2, 1.0f);

    g.setColour (s.isMouseOverOrDragging() ? col::bone : col::silver);
    g.drawLine (c.x + std::sin (ang) * br * 0.35f, c.y - std::cos (ang) * br * 0.35f,
                c.x + std::sin (ang) * (br - 2.5f), c.y - std::cos (ang) * (br - 2.5f), 1.6f);
}

void LookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const bool on = b.getToggleState();
    const bool prime = b.getProperties().getWithDefault ("prime", false);
    juce::Colour fill = on ? col::bone.withAlpha (0.10f) : col::panel;
    if (prime) fill = col::bone.withAlpha (down ? 0.28f : over ? 0.2f : 0.14f);
    else if (over) fill = fill.brighter (0.05f);
    g.setColour (fill);
    g.fillRoundedRectangle (r, 2.0f);
    g.setColour (on || prime ? col::silver : col::line.brighter (over ? 0.35f : 0.1f));
    g.drawRoundedRectangle (r, 2.0f, 1.0f);
}

void LookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    const bool on = b.getToggleState() || (bool) b.getProperties().getWithDefault ("prime", false);
    g.setColour (! b.isEnabled() ? col::line.brighter (0.4f) : on ? col::bone : col::dim.brighter (0.3f));
    g.setFont (Fonts::get().label (juce::jmin (9.5f, b.getHeight() * 0.36f), 0.18f));
    g.drawText (b.getButtonText(), b.getLocalBounds(), juce::Justification::centred);
}

juce::Label* LookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (Fonts::get().num (11.0f));
    l->setColour (juce::Label::textColourId, col::bone.withAlpha (0.85f));
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setJustificationType (juce::Justification::centred);
    return l;
}

// ============================================================ art: gothic window + fog

namespace possui
{
class Art : public juce::Component
{
public:
    Art()
    {
        juce::Random rng (11);
        for (int layer = 0; layer < 2; ++layer) fog[layer] = makeFog (384, 150, rng, layer == 0 ? 0.36f : 0.42f);
        for (auto& m : motes) { m.x = rng.nextFloat(); m.y = rng.nextFloat(); m.s = 0.4f + rng.nextFloat(); m.v = 0.0004f + 0.0012f * rng.nextFloat(); }
    }

    void tick (float intensityTarget, float leadTarget, bool leadOn)
    {
        intensity += (intensityTarget - intensity) * 0.05f;
        lead += ((leadOn ? leadTarget : 0.0f) - lead) * 0.12f;
        t += 1.0f / 30.0f;
        for (auto& m : motes) { m.y -= m.v * (1.0f + intensity); m.x += 0.0004f * std::sin (t * 0.7f + m.s * 9.0f); if (m.y < 0.0f) m.y += 1.0f; }
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        juce::Path clip; clip.addRoundedRectangle (r, 6.0f);
        g.saveState();
        g.reduceClipRegion (clip);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0c0c10), 0, r.getY(), juce::Colour (0xff040405), 0, r.getBottom(), false));
        g.fillRect (r);

        const float W = r.getWidth(), H = r.getHeight();
        const float cx = r.getCentreX();
        const float ww = W * 0.7f, top = r.getY() + H * 0.07f, bottom = r.getY() + H * 0.88f;
        const float left = cx - ww * 0.5f, right = cx + ww * 0.5f;
        const float spring = top + ww * 0.62f;

        auto arch = [] (float l, float rr, float tp, float sp, float bt)
        {
            juce::Path p;
            const float c = (l + rr) * 0.5f, w = rr - l;
            p.startNewSubPath (l, bt);
            p.lineTo (l, sp);
            p.cubicTo (l, sp - w * 0.42f, c - w * 0.16f, tp + w * 0.06f, c, tp);
            p.cubicTo (c + w * 0.16f, tp + w * 0.06f, rr, sp - w * 0.42f, rr, sp);
            p.lineTo (rr, bt);
            p.closeSubPath();
            return p;
        };

        // light through the glass
        const float glow = 0.09f + 0.3f * intensity + 0.22f * lead;
        auto glass = arch (left + 8, right - 8, top + 10, spring + 4, bottom);
        g.setGradientFill (juce::ColourGradient (col::ice.withAlpha (glow), cx, top + (bottom - top) * 0.35f,
                                                 col::ice.withAlpha (0.0f), cx, bottom + 40.0f, true));
        g.fillPath (glass);

        // light shafts falling to the floor
        {
            juce::Path shaft;
            shaft.startNewSubPath (left + 12, spring);
            shaft.lineTo (right - 12, spring);
            shaft.lineTo (right + W * 0.18f, r.getBottom());
            shaft.lineTo (left - W * 0.18f, r.getBottom());
            shaft.closeSubPath();
            g.setGradientFill (juce::ColourGradient (col::ice.withAlpha (0.04f + 0.1f * intensity), cx, spring,
                                                     juce::Colours::transparentBlack, cx, r.getBottom(), false));
            g.fillPath (shaft);
        }

        // tracery
        const auto stone = col::silver;
        auto strokeGlow = [&] (const juce::Path& p, float w)
        {
            g.setColour (stone.withAlpha (0.06f + 0.08f * intensity));
            g.strokePath (p, juce::PathStrokeType (w + 5.0f));
            g.setColour (stone.withAlpha (0.55f));
            g.strokePath (p, juce::PathStrokeType (w));
        };
        strokeGlow (arch (left, right, top, spring, bottom), 2.0f);
        strokeGlow (arch (left + 8, right - 8, top + 10, spring + 4, bottom), 1.0f);

        const float lancetTop = spring + ww * 0.32f;
        const float mid = cx;
        const float lw = (ww - 16) * 0.5f - 10.0f;
        strokeGlow (arch (left + 14, left + 14 + lw, lancetTop, lancetTop + lw * 0.55f, bottom - 6), 1.2f);
        strokeGlow (arch (right - 14 - lw, right - 14, lancetTop, lancetTop + lw * 0.55f, bottom - 6), 1.2f);
        {
            juce::Path m; m.startNewSubPath (mid, lancetTop - 4); m.lineTo (mid, bottom); strokeGlow (m, 1.2f);
        }
        // rose
        const float rr = ww * 0.21f;
        const juce::Point<float> rc (cx, spring + ww * 0.02f);
        juce::Path rose;
        rose.addEllipse (rc.x - rr, rc.y - rr, rr * 2, rr * 2);
        rose.addEllipse (rc.x - rr * 0.28f, rc.y - rr * 0.28f, rr * 0.56f, rr * 0.56f);
        for (int i = 0; i < 6; ++i)
        {
            const float a = juce::MathConstants<float>::twoPi * i / 6.0f + t * 0.02f * intensity;
            const juce::Point<float> fc (rc.x + std::sin (a) * rr * 0.58f, rc.y - std::cos (a) * rr * 0.58f);
            rose.addEllipse (fc.x - rr * 0.3f, fc.y - rr * 0.3f, rr * 0.6f, rr * 0.6f);
        }
        strokeGlow (rose, 1.1f);

        // floor line
        g.setColour (stone.withAlpha (0.25f));
        g.drawHorizontalLine ((int) bottom, left - 30, right + 30);

        // fog
        for (int layer = 0; layer < 2; ++layer)
        {
            const float speed = layer == 0 ? 6.0f : -10.0f;
            float off = std::fmod (t * speed * (1.0f + intensity), W);
            if (off < 0) off += W;
            const float fy = r.getY() + H * (layer == 0 ? 0.48f : 0.62f);
            const float fh = H * (layer == 0 ? 0.42f : 0.40f);
            g.setOpacity (juce::jlimit (0.0f, 1.0f, 0.28f + 0.6f * intensity + 0.15f * lead) * (layer == 0 ? 0.8f : 1.0f));
            g.drawImage (fog[layer], { r.getX() + off - W, fy, W, fh });
            g.drawImage (fog[layer], { r.getX() + off, fy, W, fh });
            g.setOpacity (1.0f);
        }
        // motes
        for (auto& m : motes)
        {
            const float a = (0.15f + 0.35f * intensity) * (0.5f + 0.5f * std::sin (t * 1.3f + m.s * 7.0f));
            g.setColour (col::bone.withAlpha (a * 0.6f));
            const float sz = 1.0f + m.s;
            g.fillEllipse (r.getX() + m.x * W, r.getY() + m.y * H, sz, sz);
        }

        // vignette
        g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, cx, r.getCentreY(), juce::Colour (0xcc000000), r.getX(), r.getY(), true));
        g.fillRect (r);
        g.restoreState();

        g.setColour (col::line);
        g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    }

private:
    static juce::Image makeFog (int w, int h, juce::Random& rng, float thresh)
    {
        // tileable (in x) value-noise fbm
        const int gx = 16, gy = 8;
        std::vector<float> grid ((size_t) (gx * gy));
        for (auto& v : grid) v = rng.nextFloat();
        auto at = [&] (int x, int y) { x = ((x % gx) + gx) % gx; y = juce::jlimit (0, gy - 1, y); return grid[(size_t) (y * gx + x)]; };
        auto noise = [&] (float x, float y)
        {
            const int xi = (int) std::floor (x), yi = (int) std::floor (y);
            float fx = x - xi, fy = y - yi;
            fx = fx * fx * (3 - 2 * fx); fy = fy * fy * (3 - 2 * fy);
            const float a = at (xi, yi), b = at (xi + 1, yi), c = at (xi, yi + 1), d = at (xi + 1, yi + 1);
            return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
        };
        juce::Image img (juce::Image::ARGB, w, h, true);
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                float v = 0, amp = 0.5f, fq = 1.0f;
                for (int o = 0; o < 4; ++o)
                {
                    v += amp * noise ((float) x / w * gx * fq, (float) y / h * (gy - 1) * fq);
                    amp *= 0.5f; fq *= 2.0f;
                }
                const float edge = std::sin (juce::MathConstants<float>::pi * (float) y / (float) h);   // soft top/bottom
                float a = juce::jlimit (0.0f, 1.0f, (v - thresh) * 2.4f) * edge;
                a = a * a;
                img.setPixelAt (x, y, juce::Colour::fromFloatRGBA (0.91f, 0.9f, 0.93f, a * 0.85f));
            }
        return img;
    }

    struct Mote { float x, y, s, v; };
    std::array<juce::Image, 2> fog;
    std::array<Mote, 40> motes;
    float t = 0, intensity = 0, lead = 0;
};

// ============================================================ preset card

class Card : public juce::Component, public juce::SettableTooltipClient
{
public:
    Card (PossProcessor& p, int i) : proc (p), index (i) {}

    void paint (juce::Graphics& g) override
    {
        auto& F = Fonts::get();
        const auto& pr = proc.getPreset (index);
        const bool sel = proc.getSelected() == index;
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (sel ? col::raised : col::panel);
        g.fillRoundedRectangle (r, 3.0f);
        g.setColour (sel ? col::silver : (hover ? col::line.brighter (0.45f) : col::line));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        if (pr.isEmpty()) return;

        auto in = r.reduced (12.0f, 9.0f);
        auto top = in.removeFromTop (13.0f);
        lockBox = top.removeFromRight (13.0f);
        drawLock (g, lockBox, pr.locked);
        g.setColour (col::dim);
        g.setFont (F.num (10.0f));
        g.drawText ("0" + juce::String (index + 1), top, juce::Justification::centredLeft);

        in.removeFromTop (4.0f);
        g.setColour (sel ? col::bone : col::bone.withAlpha (0.82f));
        g.setFont (F.gothic (25.0f));
        g.drawFittedText (pr.name, in.removeFromTop (28.0f).toNearestInt(), juce::Justification::centredLeft, 1, 0.75f);
        g.setColour (col::silver.withAlpha (0.6f));
        g.setFont (F.label (8.0f, 0.22f));
        g.drawText (pr.lane.toUpperCase(), in.removeFromTop (15.0f), juce::Justification::centredLeft);
        in.removeFromTop (6.0f);
        g.setColour (col::dim.brighter (0.25f));
        g.setFont (F.num (10.5f));
        g.drawFittedText (pr.desc, in.withTrimmedBottom (20.0f).toNearestInt(), juce::Justification::topLeft, 4, 1.0f);

        // the three strongest effects
        const auto& s = pr.settings;
        std::vector<std::pair<float, juce::String>> fx = {
            { s.dbl, "DBL" }, { s.chorus, "CHO" }, { s.flange, "FLG" }, { s.phase, "PHS" }, { s.wobble, "WOB" },
            { s.ghost, "GHO" }, { s.shimmer, "SHM" }, { s.dirt, "DRT" }, { s.lofi, "LOF" }, { s.autopan, "PAN" } };
        std::sort (fx.begin(), fx.end(), [] (auto& a, auto& b) { return a.first > b.first; });
        auto row = in.removeFromBottom (14.0f);
        g.setFont (F.num (9.5f));
        for (int k = 0; k < 3; ++k)
        {
            if (fx[(size_t) k].first < 0.05f) break;
            const auto txt = fx[(size_t) k].second + " " + juce::String (juce::roundToInt (fx[(size_t) k].first * 100));
            const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), txt) + 10.0f;
            auto chip = row.removeFromLeft (tw);
            row.removeFromLeft (4.0f);
            g.setColour (col::line);
            g.fillRoundedRectangle (chip, 2.0f);
            g.setColour (col::bone.withAlpha (0.8f));
            g.drawText (txt, chip, juce::Justification::centred);
        }
    }

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (! getLocalBounds().contains (e.getPosition()) || proc.getPreset (index).isEmpty()) return;
        if (lockBox.expanded (6.0f).contains (e.position)) proc.toggleLock (index);
        else proc.applyPreset (index);
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
        g.setColour (locked ? col::bone : col::dim);
        g.strokePath (sh, juce::PathStrokeType (1.3f));
        if (locked) g.fillRect (body); else g.drawRect (body, 1.1f);
    }

    PossProcessor& proc;
    int index;
    bool hover = false;
    juce::Rectangle<float> lockBox;
};
} // namespace possui

// ============================================================ editor

namespace { constexpr int W = 1120, H = 700; }

PossEditor::PossEditor (PossProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    art = std::make_unique<Art>();
    addAndMakeVisible (*art);

    const auto roles = poss::roleNames();
    const char* roleTips[] = {
        "Tight double of the lead. Stays close to center.",
        "Harmony stack. Wider, softer, sits behind the lead.",
        "Ad-libs. More character, more movement.",
        "Whispers. Thin, airy, wide and ghostly.",
        "Low layer or octave-down. Warm, centered, darker." };
    for (int i = 0; i < roles.size(); ++i)
    {
        auto b = std::make_unique<juce::TextButton> (roles[i]);
        b->setTooltip (roleTips[i]);
        b->onClick = [this, i] { proc.setRole (i); };
        addAndMakeVisible (*b);
        roleButtons.push_back (std::move (b));
    }

    for (int i = 0; i < poss::PresetBank::numSlots; ++i)
    {
        auto c = std::make_unique<Card> (proc, i);
        c->setTooltip ("Click to load. Lock to keep through REGEN.");
        addAndMakeVisible (*c);
        cards.push_back (std::move (c));
    }

    regenButton.getProperties().set ("prime", true);
    regenButton.setTooltip ("Six new presets for this role and beat. Locked ones stay.");
    regenButton.onClick = [this] { proc.regenerate(); };
    moreButton.setTooltip ("Variations of the selected preset in the unlocked slots.");
    moreButton.onClick = [this] { proc.moreLikeThis(); };
    loadButton.setTooltip ("Load the beat so presets fit it. You can also drag the file onto the window.");
    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Beat", juce::File(), "*.wav;*.mp3;*.flac;*.aif;*.aiff;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc) { if (fc.getResult().existsAsFile()) proc.analyseFile (fc.getResult()); });
    };
    for (auto* b : { &regenButton, &moreButton, &loadButton }) addAndMakeVisible (*b);

    auto addKnob = [this] (const char* id, const char* name, const char* tip)
    {
        auto k = std::make_unique<Knob>();
        k->name = name;
        k->slider.setLookAndFeel (&lnf);
        k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 66, 16);
        k->slider.setTooltip (tip);
        addAndMakeVisible (k->slider);
        k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
        knobs.push_back (std::move (k));
    };
    addKnob ("dbl",     "DOUBLE",  "Detuned double voices on each side.");
    addKnob ("chorus",  "CHORUS",  "Three-voice ensemble chorus.");
    addKnob ("flange",  "FLANGE",  "Jet-like sweep. Rate and feedback come from the preset.");
    addKnob ("phase",   "PHASE",   "Slow, breathing phaser.");
    addKnob ("wobble",  "WOBBLE",  "Warped tape pitch drift.");
    addKnob ("dirt",    "DIRT",    "Saturation and grit.");
    addKnob ("lofi",    "LO-FI",   "Rolled-off, bit-reduced, cheap and old.");
    addKnob ("ghost",   "GHOST",   "Diffuse haunted tail.");
    addKnob ("shimmer", "SHIMMER", "Octave-up glow inside the ghost tail.");
    addKnob ("width",   "WIDTH",   "How far the layer spreads out to the sides. Mono-safe.");
    addKnob ("autopan", "PAN",     "Drifts the layer side to side.");
    addKnob ("duck",    "DUCK",    "How far the layer dips while the lead sings (needs the lead on the sidechain).");
    addKnob ("bloom",   "BLOOM",   "Lead singing: the layer spreads wider. Lead silent: the ghost tail fills the gap.");
    addKnob ("mix",     "MIX",     "Dry layer vs. processed.");
    addKnob ("output",  "OUTPUT",  "Output level.");

    proc.addChangeListener (this);
    setSize (W, H);
    startTimerHz (30);
}

PossEditor::~PossEditor()
{
    proc.removeChangeListener (this);
    for (auto& k : knobs) k->slider.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void PossEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    for (auto& c : cards) c->repaint();
    moreButton.setEnabled (proc.getSelected() >= 0);
    repaint();
}

void PossEditor::timerCallback()
{
    art->tick (proc.intensity(), proc.leadActivity(), proc.leadPresent());
    const int role = proc.getRole();
    if (role != shownRole)
    {
        shownRole = role;
        for (int i = 0; i < (int) roleButtons.size(); ++i) roleButtons[(size_t) i]->setToggleState (i == role, juce::dontSendNotification);
    }
    repaint (header);
}

void PossEditor::paint (juce::Graphics& g)
{
    auto& F = Fonts::get();
    g.fillAll (col::bg);

    auto h = header;
    g.setColour (col::bone);
    g.setFont (F.gothic (46.0f));
    g.drawText ("Possession", h.removeFromLeft (230), juce::Justification::centredLeft);
    g.setColour (col::dim);
    g.setFont (F.label (8.5f, 0.35f));
    g.drawText ("VOCAL LAYER ENGINE", h.removeFromLeft (230).withTrimmedTop (10), juce::Justification::centredLeft);

    // right side: lead meter, beat readout (load button sits at far right)
    auto rt = h.withTrimmedRight (loadButton.getWidth() + 16);
    {
        auto lead = rt.removeFromRight (150).withTrimmedTop (8);
        const bool on = proc.leadPresent();
        g.setColour (col::dim);
        g.setFont (F.label (8.0f, 0.3f));
        g.drawText (on ? "LEAD" : "NO LEAD", lead.removeFromLeft (64), juce::Justification::centredLeft);
        auto bar = lead.withSizeKeepingCentre (lead.getWidth(), 4).toFloat();
        g.setColour (col::line);
        g.fillRoundedRectangle (bar, 2.0f);
        if (on)
        {
            g.setColour (col::ice);
            g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * juce::jlimit (0.0f, 1.0f, proc.leadActivity())), 2.0f);
        }
    }
    rt.removeFromRight (24);
    {
        auto bt = rt.removeFromRight (280).withTrimmedTop (8);
        const auto& b = proc.getBeat();
        g.setColour (b.valid ? col::bone : col::dim);
        g.setFont (F.label (8.0f, 0.3f));
        g.drawText (proc.isAnalysing() ? "READING BEAT" : b.valid ? b.vibe.toUpperCase() : "NO BEAT LOADED", bt, juce::Justification::centredRight);
    }

    g.setColour (col::line);
    g.drawHorizontalLine (header.getBottom() + 4, (float) header.getX(), (float) header.getRight());

    // selected preset description
    const int sel = proc.getSelected();
    g.setFont (F.label (8.0f, 0.25f));
    g.setColour (col::dim);
    const juce::String role = poss::roleNames()[proc.getRole()];
    g.drawText (sel >= 0 ? (role + "   /   " + proc.getPreset (sel).name.toUpperCase() + "   /   TWEAK BELOW")
                         : (role + "   /   PICK A PRESET   /   LOCK THE KEEPERS   /   REGEN THE REST"),
                descArea, juce::Justification::centredLeft);

    // knob labels
    g.setColour (col::line);
    g.drawHorizontalLine (knobArea.getY() - 6, (float) knobArea.getX(), (float) knobArea.getRight());
    for (auto& k : knobs)
    {
        g.setColour (col::dim);
        g.setFont (F.label (7.5f, 0.22f));
        g.drawText (k->name, k->slider.getBounds().withHeight (12).translated (0, -13), juce::Justification::centred);
    }
}

void PossEditor::paintOverChildren (juce::Graphics& g)
{
    if (! dragging) return;
    g.setColour (col::bg.withAlpha (0.85f));
    g.fillRect (getLocalBounds());
    g.setColour (col::bone);
    g.setFont (Fonts::get().gothic (40.0f));
    g.drawText ("Drop the beat", getLocalBounds(), juce::Justification::centred);
}

void PossEditor::resized()
{
    auto area = getLocalBounds().reduced (24, 16);
    header = area.removeFromTop (54);
    loadButton.setBounds (header.withTrimmedLeft (header.getWidth() - 110).withSizeKeepingCentre (110, 28).translated (0, 4));
    area.removeFromTop (16);

    knobArea = area.removeFromBottom (86);
    area.removeFromBottom (20);

    artArea = area.removeFromLeft (330);
    art->setBounds (artArea);
    area.removeFromLeft (24);

    auto roles = area.removeFromTop (30);
    auto more = roles.removeFromRight (140);
    roles.removeFromRight (6);
    auto regen = roles.removeFromRight (90);
    roles.removeFromRight (18);
    regenButton.setBounds (regen);
    moreButton.setBounds (more);
    const int rw = (roles.getWidth() - 4 * 4) / 5;
    for (auto& b : roleButtons) { b->setBounds (roles.removeFromLeft (rw)); roles.removeFromLeft (4); }

    area.removeFromTop (14);
    descArea = area.removeFromBottom (20);
    area.removeFromBottom (8);
    const int gap = 8;
    const int cw = (area.getWidth() - 2 * gap) / 3, ch = (area.getHeight() - gap) / 2;
    for (int i = 0; i < (int) cards.size(); ++i)
        cards[(size_t) i]->setBounds (area.getX() + (i % 3) * (cw + gap), area.getY() + (i / 3) * (ch + gap), cw, ch);

    auto k = knobArea.withTrimmedTop (12);
    const int kw = k.getWidth() / (int) knobs.size();
    for (auto& kn : knobs) kn->slider.setBounds (k.removeFromLeft (kw).reduced (4, 0));
}

bool PossEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
    {
        const auto e = juce::File (f).getFileExtension().toLowerCase();
        if (e == ".wav" || e == ".mp3" || e == ".flac" || e == ".aif" || e == ".aiff" || e == ".ogg") return true;
    }
    return false;
}

void PossEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dragging = false;
    for (auto& f : files) if (isInterestedInFileDrag ({ f })) { proc.analyseFile (juce::File (f)); break; }
    repaint();
}
