#include "RageEditor.h"
#include "BinaryData.h"

using namespace mime::ui;

namespace
{
    const juce::Colour pink { 0xffff4fa3 };
    const juce::Colour red  { 0xffe8364f };
    constexpr int W = 1180, H = 700;
}

RageEditor::RageEditor (RageProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    photo = juce::ImageCache::getFromMemory (BinaryData::filipina_rage_jpg, BinaryData::filipina_rage_jpgSize);

    // crunch type picker
    const auto names = rage::crunchNames(), blurbs = rage::crunchBlurbs();
    for (int i = 0; i < names.size(); ++i)
    {
        auto b = std::make_unique<juce::TextButton> (names[i].toUpperCase());
        b->setColour (juce::TextButton::buttonOnColourId, red);
        b->setTooltip (blurbs[i]);
        b->onClick = [this, i] { if (crunchAtt) crunchAtt->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (*b);
        crunchButtons.push_back (std::move (b));
    }
    crunchAtt = std::make_unique<juce::ParameterAttachment> (*proc.apvts.getParameter ("crunch"),
        [this] (float v) { currentCrunch = juce::roundToInt (v); updateCrunchButtons(); });
    crunchAtt->sendInitialUpdate();

    kAmount  = &addKnob ("amount",  "AMOUNT",   "How hard the crunch hits on each 808.", red);
    kMix     = &addKnob ("mix",     "HIT MIX",  "How much of the crunched voice you hear during a hit. 100% = fully crunched.", red);
    kSlam    = &addKnob ("slam",    "SLAM",     "Extra volume push on every hit, so the crunch jumps out with the 808.", pink);
    kIdle    = &addKnob ("idle",    "IDLE",     "Crunch between hits. 0 = clean vocal until the 808 lands.", pink);
    kAttack  = &addKnob ("attack",  "ATTACK",   "How fast the crunch grabs when the 808 hits. Short = snappy.", colours::text);
    kHold    = &addKnob ("hold",    "HOLD",     "How long the crunch stays fully on after each hit.", colours::text);
    kRelease = &addKnob ("release", "RELEASE",  "How long the crunch takes to fade back to clean.", colours::text);
    kSens    = &addKnob ("sens",    "SENSITIVITY", "Higher = quieter 808s and ghost notes trigger too. Lower = only the big hits.", colours::ghost);
    kDetect  = &addKnob ("detect",  "808 FREQ", "Where MIME listens for the 808. Hit LEARN 808 to set it automatically.", colours::ghost);
    kTone    = &addKnob ("tone",    "TONE",     "Darker (left) or brighter (right) crunch.", colours::text);
    kLowCut  = &addKnob ("lowcut",  "LOW CUT",  "Cleans rumble out of the vocal before it gets crunched.", colours::text);
    kOut     = &addKnob ("output",  "OUTPUT",   "Final level.", colours::text);

    modeBox.addItemList ({ "Hit", "Follow" }, 1);
    modeBox.setTooltip ("Hit: every 808 fires a crunch shaped by Attack / Hold / Release.\n"
                        "Follow: the crunch rides the 808's volume. Long 808s keep crunching while they ring.");
    addAndMakeVisible (modeBox);
    modeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "mode", modeBox);
    modeLabel.setText ("MODE", juce::dontSendNotification);
    modeLabel.setFont (font (11.5f, true));
    modeLabel.setColour (juce::Label::textColourId, colours::dim);
    modeLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (modeLabel);

    for (auto* b : { &trig808, &trigKick })
    {
        b->setColour (juce::TextButton::buttonOnColourId, pink);
        addAndMakeVisible (*b);
    }
    trig808.setTooltip ("Crunch on the 808: long, booming bass hits. Best for trap and rage.");
    trigKick.setTooltip ("Punch on the kick: short, fast hits. Best for dance, house and four-on-the-floor beats. Works even with a bassline under the kick.");
    trig808.onClick  = [this] { proc.setTrigger (0); };
    trigKick.onClick = [this] { proc.setTrigger (1); };

    tapButton.setColour (juce::TextButton::buttonColourId, red.withAlpha (0.9f));
    tapButton.setTooltip ("Fire a hit by hand to hear the crunch without a beat.");
    tapButton.onClick = [this] { proc.tapHit(); };
    addAndMakeVisible (tapButton);

    learnButton.setTooltip ("Play the beat and hit this. FILIPINA RAGE finds your 808 and tunes the detector to it (about 6 seconds).");
    learnButton.onClick = [this] { proc.learn808(); };
    addAndMakeVisible (learnButton);

    listenToggle.setTooltip ("Hear exactly what the detector hears (the 808 band). Handy for tuning 808 FREQ. Turn it off when you're done!");
    addAndMakeVisible (listenToggle);
    listenAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "listen", listenToggle);
    autoGainToggle.setTooltip ("Keeps the crunch at the same loudness as the clean voice. SLAM still adds its push on top.");
    addAndMakeVisible (autoGainToggle);
    autoGainAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "autogain", autoGainToggle);

    proc.addChangeListener (this);
    setSize (W, H);
    startTimerHz (30);
}

RageEditor::~RageEditor()
{
    proc.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

RageEditor::Knob& RageEditor::addKnob (const juce::String& id, const juce::String& name, const juce::String& tip, juce::Colour col)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 16);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, col);
    k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k->slider.setColour (juce::Slider::textBoxTextColourId, colours::text.withAlpha (0.85f));
    k->slider.setTooltip (tip);
    addAndMakeVisible (k->slider);
    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (font (11.0f, true));
    k->label.setColour (juce::Label::textColourId, colours::dim);
    addAndMakeVisible (k->label);
    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    knobs.push_back (std::move (k));
    return *knobs.back();
}

void RageEditor::updateCrunchButtons()
{
    for (int i = 0; i < (int) crunchButtons.size(); ++i)
        crunchButtons[(size_t) i]->setToggleState (i == currentCrunch, juce::dontSendNotification);
    repaint (blurbArea);
}

void RageEditor::timerCallback()
{
    const int hits = proc.hitCount();
    if (hits != lastHits) { flash = 1.0f; lastHits = hits; }
    else flash *= 0.86f;
    flash = juce::jmax (flash, proc.hitLevel() * 0.8f);
    scMeter = juce::jmax (proc.sidechainPeak(), scMeter * 0.85f);
    const int trig = proc.getTrigger();
    if (trig != shownTrigger)
    {
        shownTrigger = trig;
        trig808.setToggleState (trig == 0, juce::dontSendNotification);
        trigKick.setToggleState (trig == 1, juce::dontSendNotification);
        kDetect->label.setText (trig == 1 ? "KICK FREQ" : "808 FREQ", juce::dontSendNotification);
        repaint();
    }
    learnButton.setButtonText (proc.isLearning() ? "LEARNING..." : (trig == 1 ? "LEARN KICK" : "LEARN 808"));
    repaint (photoArea.expanded (24).withBottom (getHeight()));
    repaint (scopeArea);
}

void RageEditor::paint (juce::Graphics& g)
{
    using namespace colours;
    g.fillAll (bg);

    // ---- her ----
    if (photo.isValid())
    {
        auto r = photoArea.toFloat();
        // glow that flashes on every hit
        for (int i = 6; i >= 1; --i)
        {
            g.setColour (pink.withAlpha ((0.05f + 0.12f * flash) * (float) (7 - i) / 6.0f));
            g.fillRoundedRectangle (r.expanded ((float) i * 3.5f), 18.0f + (float) i * 3.5f);
        }
        juce::Path clip; clip.addRoundedRectangle (r, 16.0f);
        g.saveState();
        g.reduceClipRegion (clip);
        g.setOpacity (1.0f);
        g.drawImage (photo, r, juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
        // fade to black at the bottom for the title
        juce::ColourGradient fade (juce::Colours::transparentBlack, 0, r.getBottom() - r.getHeight() * 0.35f, bg.withAlpha (0.92f), 0, r.getBottom(), false);
        g.setGradientFill (fade);
        g.fillRect (r);
        g.restoreState();
        g.setColour (red.interpolatedWith (pink, flash).withAlpha (0.5f + 0.5f * flash));
        g.drawRoundedRectangle (r, 16.0f, 1.5f + 2.5f * flash);

        auto cap = r.removeFromBottom (64.0f).reduced (18.0f, 8.0f);
        g.setColour (text);
        g.setFont (font (13.0f, true));
        g.drawText ("dedicated to my beautiful girlfriend", cap, juce::Justification::centredLeft);
    }

    // hit counter under the photo
    {
        auto c = juce::Rectangle<int> (photoArea.getX(), photoArea.getBottom() + 18, photoArea.getWidth(), getHeight() - photoArea.getBottom() - 34);
        g.setColour (panel);
        g.fillRoundedRectangle (c.toFloat(), 10.0f);
        g.setColour (line);
        g.drawRoundedRectangle (c.toFloat().reduced (0.5f), 10.0f, 1.0f);
        auto ci = c.reduced (18, 10);
        g.setColour (pink);
        g.setFont (font (11.0f, true));
        g.drawText (shownTrigger == 1 ? "KICK HITS" : "808 HITS", ci.removeFromTop (14), juce::Justification::centredLeft);
        g.setColour (text.interpolatedWith (pink, flash));
        g.setFont (font (44.0f, true));
        g.drawText (juce::String (proc.hitCount()), ci, juce::Justification::centredLeft);
        g.setColour (dim);
        g.setFont (font (12.0f));
        g.drawText (juce::String (rage::crunchNames()[currentCrunch]).toUpperCase() + " on every hit", ci, juce::Justification::centredRight);
    }

    // ---- title ----
    auto t = titleArea;
    g.setColour (text);
    g.setFont (font (38.0f, true));
    const juce::String title = "FILIPINA RAGE";
    const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), title);
    g.drawText (title, t.removeFromLeft ((int) tw + 20), juce::Justification::centredLeft);
    g.setColour (pink);
    g.setFont (font (14.0f));
    g.drawText (shownTrigger == 1 ? "punches your vocal every time the kick hits." : "crunches your vocal every time the 808 hits.", t, juce::Justification::centredLeft);

    // sidechain meter next to TAP
    {
        auto m = tapButton.getBounds().withWidth (8).translated (-18, 0).reduced (0, 4);
        g.setColour (line);
        g.fillRoundedRectangle (m.toFloat(), 3.0f);
        const float v = juce::jlimit (0.0f, 1.0f, scMeter > 0 ? (20.0f * std::log10 (scMeter) + 48.0f) / 48.0f : 0.0f);
        g.setColour (v > 0.05f ? ghost : line);
        g.fillRoundedRectangle (m.toFloat().withTrimmedTop (m.getHeight() * (1.0f - v)), 3.0f);
    }

    // status
    g.setColour (dim);
    g.setFont (font (13.0f));
    g.drawFittedText (proc.getStatus(), statusArea, juce::Justification::centredLeft, 1);

    // ---- scope ----
    auto s = scopeArea.toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (s, 10.0f);
    g.setColour (line);
    g.drawRoundedRectangle (s.reduced (0.5f), 10.0f, 1.0f);
    {
        auto plot = s.reduced (14.0f, 12.0f);
        g.setColour (dim);
        g.setFont (font (11.0f, true));
        auto head = plot.removeFromTop (26.0f);
        g.drawText ("TRIGGER ON", head.removeFromLeft (80.0f), juce::Justification::centredLeft);
        g.setColour (ghost); g.setFont (font (11.0f));
        g.drawText (shownTrigger == 1 ? "kick level   crunch" : "808 level   - - threshold   crunch", head, juce::Justification::centredRight);
        plot.removeFromTop (6.0f);

        const int N = RageProcessor::historySize;
        const int w0 = proc.histWrite.load();
        const float dx = plot.getWidth() / (float) (N - 1);
        auto yFor = [&] (float v) { return plot.getBottom() - juce::jlimit (0.0f, 1.05f, v) * plot.getHeight(); };

        juce::Path crunch, det;
        crunch.startNewSubPath (plot.getX(), plot.getBottom());
        for (int i = 0; i < N; ++i)
        {
            const int idx = (w0 + i) % N;
            const float x = plot.getX() + dx * (float) i;
            crunch.lineTo (x, yFor (proc.histLevel[(size_t) idx].load()));
            const float dy = yFor (proc.histDetect[(size_t) idx].load());
            if (i == 0) det.startNewSubPath (x, dy); else det.lineTo (x, dy);
        }
        crunch.lineTo (plot.getRight(), plot.getBottom());
        crunch.closeSubPath();
        g.setGradientFill (juce::ColourGradient (red.withAlpha (0.55f), 0, plot.getY(), red.withAlpha (0.08f), 0, plot.getBottom(), false));
        g.fillPath (crunch);
        g.setColour (ghost);
        g.strokePath (det, juce::PathStrokeType (1.6f));

        const float thv = proc.histThreshold.load();
        const float ty = yFor (juce::jmax (0.0f, thv));
        if (thv >= 0.0f && shownTrigger != 1)
        {
        juce::Path th; th.startNewSubPath (plot.getX(), ty); th.lineTo (plot.getRight(), ty);
        juce::Path dashed; const float dash[] = { 6.0f, 5.0f };
        juce::PathStrokeType (1.2f).createDashedStroke (dashed, th, dash, 2);
        g.setColour (pink.withAlpha (0.8f));
        g.fillPath (dashed);
        }
    }

    // crunch blurb
    g.setColour (text.withAlpha (0.85f));
    g.setFont (font (14.0f));
    g.drawText (rage::crunchBlurbs()[currentCrunch], blurbArea, juce::Justification::centredLeft);

    // knob groups
    auto group = [&] (juce::Rectangle<int> r, const juce::String& name)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r.toFloat(), 10.0f);
        g.setColour (line);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.0f, 1.0f);
        g.setColour (pink);
        g.setFont (font (11.0f, true));
        g.drawText (name, r.reduced (12, 6).removeFromTop (14), juce::Justification::centredLeft);
    };
    group (hitGroup, "THE HIT");
    group (envGroup, "SHAPE");
    group (detGroup, shownTrigger == 1 ? "DETECT THE KICK" : "DETECT THE 808");
    group (outGroup, "OUTPUT");
}

void RageEditor::resized()
{
    auto area = getLocalBounds().reduced (18, 16);

    photoArea = area.removeFromLeft (330).withTrimmedTop (4).withHeight (330 * 1.32f > area.getHeight() ? area.getHeight() : (int) (330 * 1.32f));
    area.removeFromLeft (26);

    auto header = area.removeFromTop (50);
    tapButton.setBounds (header.removeFromRight (130).reduced (0, 7));
    header.removeFromRight (30);
    titleArea = header;
    area.removeFromTop (2);
    statusArea = area.removeFromTop (20);
    area.removeFromTop (8);
    scopeArea = area.removeFromTop (140);
    {
        auto tb = scopeArea.reduced (14, 12).removeFromTop (26).withTrimmedLeft (80);
        trig808.setBounds (tb.removeFromLeft (70));
        tb.removeFromLeft (6);
        trigKick.setBounds (tb.removeFromLeft (70));
    }
    area.removeFromTop (12);

    auto crunchRow = area.removeFromTop (34);
    const int bw = (crunchRow.getWidth() - 8 * 6) / 9;
    for (auto& b : crunchButtons) { b->setBounds (crunchRow.removeFromLeft (bw)); crunchRow.removeFromLeft (6); }
    blurbArea = area.removeFromTop (30);
    area.removeFromTop (6);

    auto row1 = area.removeFromTop ((area.getHeight() - 10) / 2);
    area.removeFromTop (10);
    auto row2 = area;

    hitGroup = row1.removeFromLeft (row1.getWidth() * 4 / 7 - 5);
    row1.removeFromLeft (10);
    envGroup = row1;
    detGroup = row2.removeFromLeft (row2.getWidth() * 4 / 7 - 5);
    row2.removeFromLeft (10);
    outGroup = row2;

    auto layoutKnobs = [] (juce::Rectangle<int> r, std::initializer_list<Knob*> ks)
    {
        r = r.reduced (8, 6).withTrimmedTop (16);
        const int kw = r.getWidth() / (int) ks.size();
        for (auto* k : ks)
        {
            auto kr = r.removeFromLeft (kw);
            k->label.setBounds (kr.removeFromTop (14));
            k->slider.setBounds (kr);
        }
    };
    layoutKnobs (hitGroup, { kAmount, kMix, kSlam, kIdle });
    layoutKnobs (envGroup, { kAttack, kHold, kRelease });

    {
        auto r = detGroup.reduced (8, 6).withTrimmedTop (16);
        const int kw = r.getWidth() / 4;
        for (auto* k : { kSens, kDetect })
        {
            auto kr = r.removeFromLeft (kw);
            k->label.setBounds (kr.removeFromTop (14));
            k->slider.setBounds (kr);
        }
        auto col = r.reduced (10, 4);
        modeLabel.setBounds (col.removeFromTop (14));
        modeBox.setBounds (col.removeFromTop (26).withWidth (juce::jmin (col.getWidth(), 120)));
        col.removeFromTop (8);
        learnButton.setBounds (col.removeFromTop (30).withWidth (juce::jmin (col.getWidth(), 150)));
        col.removeFromTop (6);
        listenToggle.setBounds (col.removeFromTop (24));
    }
    {
        auto r = outGroup.reduced (8, 6).withTrimmedTop (16);
        auto right = r.removeFromRight (110);
        const int kw = r.getWidth() / 3;
        for (auto* k : { kTone, kLowCut, kOut })
        {
            auto kr = r.removeFromLeft (kw);
            k->label.setBounds (kr.removeFromTop (14));
            k->slider.setBounds (kr);
        }
        autoGainToggle.setBounds (right.withSizeKeepingCentre (110, 24));
    }
}
