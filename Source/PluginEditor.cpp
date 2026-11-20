#include "PluginEditor.h"
#include "BinaryData.h"

using namespace mime::ui;

namespace
{
    constexpr int W = 1260, H = 680;
}

MimeEditor::MimeEditor (MimeProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&lnf);
    mimeGuy = juce::ImageCache::getFromMemory (BinaryData::mime_guy_png, BinaryData::mime_guy_pngSize);

    // ---- top buttons ----
    listenButton.setColour (juce::TextButton::buttonColourId, colours::accent.withAlpha (0.85f));
    listenButton.setTooltip ("Play your beat into MIME's sidechain and hit this. MIME listens for up to 20s, then builds presets for it. Click again to finish early.");
    listenButton.onClick = [this]
    {
        if (proc.isListening()) proc.stopListening();
        else proc.startListening();
        refresh();
    };
    addAndMakeVisible (listenButton);

    loadButton.setTooltip ("Pick your beat as an audio file (WAV, MP3, FLAC, AIFF). You can also just drag the file onto MIME.");
    loadButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Pick your beat", juce::File(), "*.wav;*.mp3;*.flac;*.aif;*.aiff;*.ogg");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  auto f = fc.getResult();
                                  if (f.existsAsFile()) proc.analyseFile (f);
                              });
    };
    addAndMakeVisible (loadButton);

    // ---- presets ----
    for (int i = 0; i < (int) cards.size(); ++i)
    {
        cards[(size_t) i].onSelect     = [this, i] { proc.applyPreset (i); };
        cards[(size_t) i].onToggleLock = [this, i] { proc.toggleLock (i); };
        addAndMakeVisible (cards[(size_t) i]);
    }

    regenButton.setTooltip ("Roll a fresh set of presets for this beat. Locked presets stay put.");
    regenButton.onClick = [this] { proc.regenerate(); };
    addAndMakeVisible (regenButton);

    moreButton.setTooltip ("Fill the unlocked slots with variations of the selected preset: dirtier, cleaner, wider, darker...");
    moreButton.onClick = [this] { proc.moreLikeThis(); };
    addAndMakeVisible (moreButton);

    details.setMultiLine (true, true);
    details.setReadOnly (true);
    details.setScrollbarsShown (true);
    details.setCaretVisible (false);
    details.setFont (font (14.0f));
    details.setJustification (juce::Justification::topLeft);
    addAndMakeVisible (details);

    // ---- controls ----
    auto setupBox = [this] (juce::ComboBox& box, juce::Label& lbl, const juce::String& name, const juce::StringArray& items, const juce::String& tip)
    {
        box.addItemList (items, 1);
        box.setTooltip (tip);
        addAndMakeVisible (box);
        lbl.setText (name, juce::dontSendNotification);
        lbl.setFont (font (11.5f, true));
        lbl.setColour (juce::Label::textColourId, colours::dim);
        addAndMakeVisible (lbl);
    };
    setupBox (styleBox, styleLabel, "STYLE", mime::styleNames(),
              "The main flavor of distortion on the middle of your voice.\n"
              "Tube: warm and thick.  Tape: soft and rounded.  Clip: blown speaker.\n"
              "Fold: metallic ring.  Crush: digital/lo-fi.  Fuzz: buzzy pedal.");
    setupBox (textureBox, textureLabel, "TEXTURE", mime::textureNames(),
              "What happens to the very top (breath, S's, air).\n"
              "Sheen: polish.  Crush: digital sparkle.  Octave: glassy shimmer.  Fold: metallic.");
    styleAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "style", styleBox);
    textureAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "texture", textureBox);

    autoGainButton.setTooltip ("Keeps the output the same loudness as the input, so louder never tricks you into thinking it sounds better.");
    addAndMakeVisible (autoGainButton);
    autoGainAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "autogain", autoGainButton);

    const auto red = colours::accent, lilac = colours::ghost, white = colours::text;
    addKnob ("drive",  "DRIVE",   "How hard the main distortion is pushed.", red);
    addKnob ("mix",    "MIX",     "Blend of distorted vs clean voice. Lower = words stay clearer.", red);
    addKnob ("body",   "BODY",    "Warmth and weight in the low end of your voice. The lows never get harsh.", white);
    addKnob ("air",    "AIR",     "How much texture goes on the top end (breath, S's, air).", lilac);
    addKnob ("focus",  "FOCUS",   "Narrows the distortion down. All the way up = tiny busted speaker / radio.", white);
    addKnob ("haze",   "HAZE",    "Smears the distortion into a ghostly, diffuse tail around the voice.", lilac);
    addKnob ("width",  "WIDTH",   "Spreads the distortion out to the sides. The clean voice stays centered and it's mono-safe.", lilac);
    addKnob ("tone",   "TONE",    "Darker (left) or brighter (right) distortion.", white);
    addKnob ("fizz",   "FIZZ",    "Automatically turns down harsh fizz only when it builds up. MIME aims it at where your beat is crowded.", white);
    addKnob ("lowcut", "LOW CUT", "Cleans up rumble and mud below this point before anything gets distorted.", white);
    addKnob ("output", "OUTPUT",  "Final level.", white);

    proc.addChangeListener (this);
    setSize (W, H);
    refresh();
    startTimerHz (20);
}

MimeEditor::~MimeEditor()
{
    proc.removeChangeListener (this);
    setLookAndFeel (nullptr);
}

void MimeEditor::addKnob (const juce::String& id, const juce::String& name, const juce::String& tip, juce::Colour col)
{
    auto k = std::make_unique<Knob>();
    k->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    k->slider.setColour (juce::Slider::rotarySliderFillColourId, col);
    k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k->slider.setColour (juce::Slider::textBoxTextColourId, colours::text.withAlpha (0.85f));
    k->slider.setTooltip (tip);
    k->slider.setPopupDisplayEnabled (false, false, nullptr);
    addAndMakeVisible (k->slider);
    k->label.setText (name, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (font (11.5f, true));
    k->label.setColour (juce::Label::textColourId, colours::dim);
    addAndMakeVisible (k->label);
    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    knobs.push_back (std::move (k));
}

void MimeEditor::refresh()
{
    const int sel = proc.getSelected();
    for (int i = 0; i < (int) cards.size(); ++i)
        cards[(size_t) i].setPreset (proc.getPreset (i), i == sel);

    if (sel >= 0)
    {
        const auto& pr = proc.getPreset (sel);
        details.clear();
        details.setColour (juce::TextEditor::textColourId, colours::text);
        details.setFont (font (18.0f, true));
        details.insertTextAtCaret (pr.name.toUpperCase() + "\n");
        details.setColour (juce::TextEditor::textColourId, colours::ghost);
        details.setFont (font (13.5f));
        details.insertTextAtCaret ("lane: " + pr.lane + "\n\n");
        details.setColour (juce::TextEditor::textColourId, colours::text);
        details.setFont (font (14.0f));
        details.insertTextAtCaret (pr.longDesc);
        details.setCaretPosition (0);
    }
    else
    {
        details.setColour (juce::TextEditor::textColourId, colours::text);
        details.setFont (font (14.0f));
        details.setText ("Click a preset to load it.\n\n"
                         "- LISTEN / LOAD BEAT FILE: MIME reads your beat.\n"
                         "- Each card is a different kind of distortion, built for that beat.\n"
                         "- Click the lock on a card to keep it, then REGENERATE to roll new ones.\n"
                         "- Select one and hit MORE LIKE THIS for variations of it.\n"
                         "- Hover anything for a plain-English explanation.", false);
    }

    moreButton.setEnabled (sel >= 0);
    listenButton.setButtonText (proc.isListening()
        ? "LISTENING  " + juce::String ((int) proc.listenProgressSeconds()) + "s / " + juce::String ((int) MimeProcessor::listenTargetSeconds) + "s  (click to finish)"
        : (proc.isAnalysingFile() ? "READING FILE..." : "LISTEN"));
    listenButton.setToggleState (proc.isListening(), juce::dontSendNotification);
    repaint();
}

void MimeEditor::changeListenerCallback (juce::ChangeBroadcaster*) { refresh(); }

void MimeEditor::timerCallback()
{
    const float lvl = proc.sidechainLevel();
    scMeter = juce::jmax (lvl, scMeter * 0.85f);
    const bool busy = proc.isListening() || proc.isAnalysingFile();
    pulse = busy ? pulse + 0.18f : 0.0f;
    if (busy) { refresh(); repaint (mimeArea); }
    else repaint (headerArea);
}

void MimeEditor::paint (juce::Graphics& g)
{
    using namespace colours;
    g.fillAll (bg);

    // header
    auto h = headerArea;
    g.setColour (text);
    g.setFont (font (40.0f, true));
    g.drawText ("M I M E", h.removeFromLeft (190), juce::Justification::centredLeft);
    g.setColour (dim);
    g.setFont (font (13.0f));
    g.drawText ("reads the beat.  makes the voice shine.", h.removeFromLeft (300), juce::Justification::centredLeft);

    // sidechain meter
    {
        auto m = listenButton.getBounds().withWidth (8).translated (-18, 0).reduced (0, 4);
        g.setColour (line);
        g.fillRoundedRectangle (m.toFloat(), 3.0f);
        const float v = juce::jlimit (0.0f, 1.0f, scMeter > 0.0f ? (20.0f * std::log10 (scMeter) + 48.0f) / 48.0f : 0.0f);
        auto fill = m.toFloat().withTrimmedTop (m.getHeight() * (1.0f - v));
        g.setColour (v > 0.05f ? ghost : line);
        g.fillRoundedRectangle (fill, 3.0f);
    }

    // the mime
    if (mimeGuy.isValid())
    {
        auto m = mimeArea.toFloat();
        const float glow = 0.22f + (pulse > 0.0f ? 0.18f * (0.5f + 0.5f * std::sin (pulse)) : 0.0f) + 0.25f * proc.getHeat();
        juce::ColourGradient grad (accent.withAlpha (glow), m.getCentreX(), m.getY() + m.getHeight() * 0.42f,
                                   accent.withAlpha (0.0f), m.getCentreX(), m.getY() - m.getHeight() * 0.15f, true);
        g.setGradientFill (grad);
        g.fillEllipse (m.reduced (-10.0f, 10.0f));

        const float scale = juce::jmin (m.getWidth() / (float) mimeGuy.getWidth(), m.getHeight() / (float) mimeGuy.getHeight());
        const float iw = mimeGuy.getWidth() * scale, ih = mimeGuy.getHeight() * scale;
        juce::Rectangle<float> dst (m.getCentreX() - iw * 0.5f, m.getBottom() - ih, iw, ih);
        g.drawImage (mimeGuy, dst, juce::RectanglePlacement::stretchToFit);

        // fade his legs into the controls panel
        juce::ColourGradient fade (bg.withAlpha (0.0f), 0, dst.getBottom() - ih * 0.18f, bg, 0, dst.getBottom(), false);
        g.setGradientFill (fade);
        g.fillRect (dst.withTop (dst.getBottom() - ih * 0.18f));
    }

    // beat panel
    auto b = beatArea.toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (b, 10.0f);
    g.setColour (line);
    g.drawRoundedRectangle (b.reduced (0.5f), 10.0f, 1.0f);

    auto bi = beatArea.reduced (16, 8);
    const auto& prof = proc.getProfile();
    g.setColour (dim);
    g.setFont (font (11.0f, true));
    g.drawText ("WHAT MIME HEARD", bi.removeFromTop (14), juce::Justification::centredLeft);
    if (prof.valid)
    {
        auto row = bi;
        g.setColour (ghost);
        g.setFont (font (18.0f, true));
        const float vw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), prof.vibe) + 18.0f;
        g.drawText (prof.vibe, row.removeFromLeft ((int) vw), juce::Justification::centredLeft);
        float x = (float) row.getX();
        g.setFont (font (12.0f, true));
        for (auto& t : prof.tags)
        {
            const float tw = juce::GlyphArrangement::getStringWidth (g.getCurrentFont(), t) + 16.0f;
            if (x + tw > (float) row.getRight()) break;
            juce::Rectangle<float> chip (x, (float) row.getCentreY() - 11.0f, tw, 22.0f);
            g.setColour (panel2);
            g.fillRoundedRectangle (chip, 6.0f);
            g.setColour (text.withAlpha (0.85f));
            g.drawText (t, chip, juce::Justification::centred);
            x += tw + 6.0f;
        }
    }
    else
    {
        g.setColour (text.withAlpha (0.8f));
        g.setFont (font (15.0f));
        g.drawText ("Nothing yet. Hit LISTEN while the beat plays into the sidechain, or drag your beat file anywhere onto MIME.",
                    bi, juce::Justification::centredLeft);
    }

    // status line
    g.setColour (dim);
    g.setFont (font (13.0f));
    g.drawFittedText (proc.getStatus(), statusArea, juce::Justification::centredLeft, 1);

    // details panel background
    auto d = details.getBounds().expanded (10).toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (d, 10.0f);
    g.setColour (line);
    g.drawRoundedRectangle (d.reduced (0.5f), 10.0f, 1.0f);

    // controls panel
    auto c = controlsArea.toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (c, 10.0f);
    g.setColour (line);
    g.drawRoundedRectangle (c.reduced (0.5f), 10.0f, 1.0f);

    // heat bar along the top of the controls
    const float heat = proc.getHeat();
    auto hb = controlsArea.toFloat().reduced (14.0f, 0).withHeight (2.0f).translated (0, 1.0f);
    g.setColour (accent.withAlpha (0.25f + 0.75f * heat));
    g.fillRect (hb.withWidth (hb.getWidth() * juce::jmax (0.04f, heat)));
}

void MimeEditor::paintOverChildren (juce::Graphics& g)
{
    if (! dragging) return;
    using namespace colours;
    g.setColour (bg.withAlpha (0.82f));
    g.fillRect (getLocalBounds());
    auto r = getLocalBounds().reduced (30).toFloat();
    g.setColour (ghost);
    const float dash[] = { 10.0f, 8.0f };
    juce::Path p; p.addRoundedRectangle (r, 16.0f);
    juce::Path dashed;
    juce::PathStrokeType (2.0f).createDashedStroke (dashed, p, dash, 2);
    g.fillPath (dashed);
    g.setFont (font (28.0f, true));
    g.drawText ("DROP THE BEAT", r, juce::Justification::centred);
}

void MimeEditor::resized()
{
    auto area = getLocalBounds().reduced (18, 14);

    headerArea = area.removeFromTop (54);
    {
        auto h = headerArea;
        listenButton.setBounds (h.removeFromRight (300).reduced (0, 8));
        h.removeFromRight (30);
        loadButton.setBounds (h.removeFromRight (160).reduced (0, 8));
    }

    area.removeFromTop (6);
    controlsArea = area.removeFromBottom (150);
    area.removeFromBottom (12);
    mimeArea = area.removeFromLeft (230);
    area.removeFromLeft (14);
    beatArea = area.removeFromTop (56);
    area.removeFromTop (6);
    statusArea = area.removeFromTop (22);
    area.removeFromTop (6);

    auto actions = area.removeFromBottom (38);
    area.removeFromBottom (10);

    auto right = area.removeFromRight (320);
    area.removeFromRight (22);
    details.setBounds (right.reduced (10));

    // 3 x 2 grid
    const int gap = 10;
    const int cw = (area.getWidth() - 2 * gap) / 3;
    const int ch = (area.getHeight() - gap) / 2;
    for (int i = 0; i < (int) cards.size(); ++i)
    {
        const int col = i % 3, row = i / 3;
        cards[(size_t) i].setBounds (area.getX() + col * (cw + gap), area.getY() + row * (ch + gap), cw, ch);
    }

    regenButton.setBounds (actions.removeFromLeft (200));
    actions.removeFromLeft (10);
    moreButton.setBounds (actions.removeFromLeft (200));

    // controls
    auto c = controlsArea.reduced (14, 12);
    auto left = c.removeFromLeft (150);
    styleLabel.setBounds (left.removeFromTop (16));
    styleBox.setBounds (left.removeFromTop (28));
    left.removeFromTop (8);
    textureLabel.setBounds (left.removeFromTop (16));
    textureBox.setBounds (left.removeFromTop (28));
    left.removeFromTop (8);
    autoGainButton.setBounds (left.removeFromTop (24));

    c.removeFromLeft (10);
    const int kw = c.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto kr = c.removeFromLeft (kw);
        k->label.setBounds (kr.removeFromTop (16));
        k->slider.setBounds (kr);
    }
}

bool MimeEditor::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files)
    {
        auto ext = juce::File (f).getFileExtension().toLowerCase();
        if (ext == ".wav" || ext == ".mp3" || ext == ".flac" || ext == ".aif" || ext == ".aiff" || ext == ".ogg")
            return true;
    }
    return false;
}

void MimeEditor::filesDropped (const juce::StringArray& files, int, int)
{
    dragging = false;
    for (auto& f : files)
        if (isInterestedInFileDrag ({ f })) { proc.analyseFile (juce::File (f)); break; }
    repaint();
}
