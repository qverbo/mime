#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace mime;

namespace
{
    juce::NormalisableRange<float> freqRange (float lo, float hi)
    {
        juce::NormalisableRange<float> r (lo, hi);
        r.setSkewForCentre (std::sqrt (lo * hi));
        return r;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout MimeProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; });
    auto hz  = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz"; });
    auto db  = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; });
    auto bip = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + String (roundToInt (v * 100.0f)); });

    Settings d;
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Style", styleNames(), d.style));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "drive", 1 }, "Drive", NormalisableRange<float> (0, 1), d.drive, pct));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "texture", 1 }, "Texture", textureNames(), d.texture));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "air", 1 }, "Air", NormalisableRange<float> (0, 1), d.air, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "body", 1 }, "Body", NormalisableRange<float> (0, 1), d.body, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "xlow", 1 }, "Low Split", freqRange (120, 800), d.xLow, hz));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "xhigh", 1 }, "High Split", freqRange (2000, 9000), d.xHigh, hz));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "focus", 1 }, "Focus", NormalisableRange<float> (0, 1), d.focus, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "haze", 1 }, "Haze", NormalisableRange<float> (0, 1), d.haze, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "width", 1 }, "Width", NormalisableRange<float> (0, 1), d.width, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "fizz", 1 }, "Fizz Control", NormalisableRange<float> (0, 1), d.fizz, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "fizzfreq", 1 }, "Fizz Freq", freqRange (1000, 9000), d.fizzFreq, hz));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "tone", 1 }, "Tone", NormalisableRange<float> (-1, 1), d.tone, bip));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0, 1), d.mix, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "lowcut", 1 }, "Low Cut", freqRange (20, 300), d.lowCut, hz));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { "autogain", 1 }, "Auto Gain", d.autoGain));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "output", 1 }, "Output", NormalisableRange<float> (-24, 12), d.outputDb, db));
    return { p.begin(), p.end() };
}

MimeProcessor::MimeProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    fifoL.assign ((size_t) fifo.getTotalSize(), 0.0f);
    fifoR.assign ((size_t) fifo.getTotalSize(), 0.0f);
    presets.generate (profile);
    status = "Route your beat to MIME's sidechain and hit LISTEN, or just drag the beat file onto MIME.";
    startTimerHz (30);
}

MimeProcessor::~MimeProcessor()
{
    stopTimer();
    cancelFile = true;
    pool.removeAllJobs (true, 4000);
}

bool MimeProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in  = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (in != out) return false;

    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! (sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo()))
            return false;
    }
    return true;
}

void MimeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, juce::jmax (1, getMainBusNumOutputChannels()));
    setLatencySamples (engine.getLatencySamples());
}

Settings MimeProcessor::readSettings() const
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    Settings s;
    s.style    = (int) v ("style");
    s.drive    = v ("drive");
    s.texture  = (int) v ("texture");
    s.air      = v ("air");
    s.body     = v ("body");
    s.xLow     = v ("xlow");
    s.xHigh    = v ("xhigh");
    s.focus    = v ("focus");
    s.haze     = v ("haze");
    s.width    = v ("width");
    s.fizz     = v ("fizz");
    s.fizzFreq = v ("fizzfreq");
    s.tone     = v ("tone");
    s.mix      = v ("mix");
    s.lowCut   = v ("lowcut");
    s.autoGain = v ("autogain") > 0.5f;
    s.outputDb = v ("output");
    return s;
}

void MimeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // clear any output channels that have no input
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    // ---- sidechain: the beat ----
    if (getBusCount (true) > 1)
    {
        if (auto* scBus = getBus (true, 1); scBus != nullptr && scBus->isEnabled() && scBus->getNumberOfChannels() > 0)
        {
            auto sc = getBusBuffer (buffer, true, 1);
            const int n = sc.getNumSamples();
            const float* l = sc.getReadPointer (0);
            const float* r = sc.getNumChannels() > 1 ? sc.getReadPointer (1) : l;

            const float mag = sc.getMagnitude (0, n);
            scLevel.store (juce::jmax (mag, scLevel.load() * 0.85f));

            if (listening.load())
            {
                int s1, n1, s2, n2;
                fifo.prepareToWrite (n, s1, n1, s2, n2);
                if (n1 > 0) { std::copy (l, l + n1, fifoL.begin() + s1); std::copy (r, r + n1, fifoR.begin() + s1); }
                if (n2 > 0) { std::copy (l + n1, l + n1 + n2, fifoL.begin() + s2); std::copy (r + n1, r + n1 + n2, fifoR.begin() + s2); }
                fifo.finishedWrite (n1 + n2);
            }
        }
    }

    auto main = getBusBuffer (buffer, false, 0);
    engine.setSettings (readSettings());
    engine.process (main);
}

// ---------------------------------------------------------------- listening

void MimeProcessor::startListening()
{
    if (analysingFile.load()) return;
    fifo.reset();
    liveAnalyzer.start (getSampleRate() > 0 ? getSampleRate() : 44100.0);
    heardSeconds = 0.0f;
    listening = true;
    setStatus ("Listening... play your beat (the loudest, fullest section is best).");
}

void MimeProcessor::stopListening()
{
    if (! listening.load()) return;
    if (heardSeconds >= listenMinimumSeconds)
    {
        finishListening();
    }
    else
    {
        listening = false;
        setStatus (heardSeconds <= 0.01f
                       ? "MIME didn't hear anything on the sidechain. Route the beat to MIME's sidechain, or drag the beat file onto MIME."
                       : "Not enough of the beat heard yet. Let it play at least 4 seconds.");
    }
}

void MimeProcessor::finishListening()
{
    listening = false;
    auto p = liveAnalyzer.finish();
    if (! p.valid)
    {
        setStatus ("Couldn't get a read on the beat. Try a louder section, or drop the beat file in.");
        return;
    }
    profile = p;
    presets.generate (profile);
    selected = -1;
    setStatus ("Heard " + juce::String (juce::roundToInt (p.seconds)) + "s of the beat. Here are presets built for it.");
}

void MimeProcessor::timerCallback()
{
    if (! listening.load()) return;

    const int ready = fifo.getNumReady();
    if (ready > 0)
    {
        drainL.resize ((size_t) ready); drainR.resize ((size_t) ready);
        int s1, n1, s2, n2;
        fifo.prepareToRead (ready, s1, n1, s2, n2);
        std::copy (fifoL.begin() + s1, fifoL.begin() + s1 + n1, drainL.begin());
        std::copy (fifoR.begin() + s1, fifoR.begin() + s1 + n1, drainR.begin());
        if (n2 > 0)
        {
            std::copy (fifoL.begin() + s2, fifoL.begin() + s2 + n2, drainL.begin() + n1);
            std::copy (fifoR.begin() + s2, fifoR.begin() + s2 + n2, drainR.begin() + n1);
        }
        fifo.finishedRead (n1 + n2);
        liveAnalyzer.push (drainL.data(), drainR.data(), n1 + n2);
        heardSeconds = liveAnalyzer.secondsHeard();
        sendChangeMessage();
    }

    if (heardSeconds >= listenTargetSeconds)
        finishListening();
}

// ---------------------------------------------------------------- file analysis

void MimeProcessor::analyseFile (const juce::File& file)
{
    if (analysingFile.load()) return;
    if (listening.load()) listening = false;

    analysingFile = true;
    cancelFile = false;
    setStatus ("Reading " + file.getFileName() + "...");

    juce::WeakReference<MimeProcessor> weak (this);
    pool.addJob ([weak, file, this]
    {
        juce::String err;
        auto p = BeatAnalyzer::analyseFile (file, cancelFile, err);
        juce::MessageManager::callAsync ([weak, p, err, name = file.getFileName()]
        {
            auto* self = weak.get();
            if (self == nullptr) return;
            self->analysingFile = false;
            if (! p.valid) { self->setStatus (err.isNotEmpty() ? err : "Couldn't analyse that file."); return; }
            self->profile = p;
            self->presets.generate (p);
            self->selected = -1;
            self->setStatus ("Analysed " + name + ". Here are presets built for it.");
        });
    });
}

// ---------------------------------------------------------------- presets

void MimeProcessor::regenerate()
{
    presets.generate (profile);
    if (selected >= 0 && ! presets.slots[(size_t) selected].locked) selected = -1;
    setStatus ("Fresh set rolled. Lock the ones you like and keep rolling.");
}

void MimeProcessor::moreLikeThis()
{
    if (selected < 0) { setStatus ("Pick a preset first, then hit MORE LIKE THIS."); return; }
    presets.moreLikeThis (selected, profile);
    setStatus ("Variations of " + presets.slots[(size_t) selected].name + " rolled into the unlocked slots.");
}

void MimeProcessor::toggleLock (int index)
{
    if (index < 0 || index >= PresetGenerator::numSlots) return;
    auto& p = presets.slots[(size_t) index];
    p.locked = ! p.locked;
    sendChangeMessage();
}

void MimeProcessor::applyPreset (int index)
{
    if (index < 0 || index >= PresetGenerator::numSlots) return;
    const auto& pr = presets.slots[(size_t) index];
    if (pr.isEmpty()) return;
    const auto& s = pr.settings;

    auto set = [this] (const char* id, float value)
    {
        if (auto* prm = apvts.getParameter (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
            prm->endChangeGesture();
        }
    };
    set ("style", (float) s.style);     set ("drive", s.drive);
    set ("texture", (float) s.texture); set ("air", s.air);
    set ("body", s.body);               set ("xlow", s.xLow);
    set ("xhigh", s.xHigh);             set ("focus", s.focus);
    set ("haze", s.haze);               set ("width", s.width);
    set ("fizz", s.fizz);               set ("fizzfreq", s.fizzFreq);
    set ("tone", s.tone);               set ("mix", s.mix);
    set ("lowcut", s.lowCut);           set ("autogain", s.autoGain ? 1.0f : 0.0f);

    selected = index;
    setStatus ("Loaded " + pr.name + ". Tweak the knobs below to taste.");
}

// ---------------------------------------------------------------- state

void MimeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::ValueTree root ("MIME");
    root.setProperty ("version", 1, nullptr);
    root.setProperty ("selected", selected, nullptr);
    root.appendChild (apvts.copyState(), nullptr);
    root.appendChild (profile.toValueTree(), nullptr);
    root.appendChild (presets.toValueTree(), nullptr);
    if (auto xml = root.createXml())
        copyXmlToBinary (*xml, dest);
}

void MimeProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr) return;
    auto root = juce::ValueTree::fromXml (*xml);
    if (! root.hasType ("MIME")) return;

    auto params = root.getChildWithName (apvts.state.getType());
    if (params.isValid()) apvts.replaceState (params);
    profile = BeatProfile::fromValueTree (root.getChildWithName ("BeatProfile"));
    presets.fromValueTree (root.getChildWithName ("Presets"));
    selected = root.getProperty ("selected", -1);
    status = profile.valid ? "Welcome back. Your beat read and presets are saved with the project." : status;
    sendChangeMessage();
}

juce::AudioProcessorEditor* MimeProcessor::createEditor() { return new MimeEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MimeProcessor(); }
