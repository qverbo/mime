#include "HexProcessor.h"
#include "HexEditor.h"

using namespace hex;

juce::AudioProcessorValueTreeState::ParameterLayout HexProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; });
    auto ms  = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + "ms"; });
    auto db  = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + String (v, 1) + "dB"; });
    auto bpm = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1); });

    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "mix", 1 }, "Mix", NormalisableRange<float> (0, 1), 1.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "chaos", 1 }, "Chaos", NormalisableRange<float> (0, 1), 0.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "density", 1 }, "Density", NormalisableRange<float> (0.05f, 1), 0.5f, pct));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Style", styleNames(), 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "grid", 1 }, "Grid", StringArray { "1/8", "1/16", "1/32" }, 1));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "repeats", 1 }, "Repeats", StringArray { "x2", "x3", "x4", "x6", "x8" }, 1));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "smooth", 1 }, "Smooth", NormalisableRange<float> (0.5f, 20.0f, 0.0f, 0.5f), 4.0f, ms));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "swing", 1 }, "Swing", NormalisableRange<float> (0, 1), 0.0f, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "output", 1 }, "Output", NormalisableRange<float> (-24, 12), 0.0f, db));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "freebpm", 1 }, "Free BPM", NormalisableRange<float> (60, 200), 140.0f, bpm));
    return { p.begin(), p.end() };
}

HexProcessor::HexProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout()),
      rng (juce::Time::currentTimeMillis())
{
    regenerate();
}

bool HexProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && l.getMainInputChannelSet() == out;
}

void HexProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, juce::jmax (1, getMainBusNumOutputChannels()));
}

HexSettings HexProcessor::readSettings() const
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    HexSettings s;
    s.mix = v ("mix");   s.chaos = v ("chaos");   s.grid = (int) v ("grid");
    s.repeats = (int) v ("repeats");   s.smoothMs = v ("smooth");   s.swing = v ("swing");
    s.outputDb = v ("output");
    return s;
}

void HexProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    engine.setSettings (readSettings());

    bool playing = false, haveHost = false;
    double bpm = apvts.getRawParameterValue ("freebpm")->load(), ppq = 0.0;

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto hostPpq = pos->getPpqPosition())
            {
                haveHost = true;
                if (auto b = pos->getBpm()) bpm = *b;
                ppq = *hostPpq;
                playing = pos->getIsPlaying();
            }

    if (wrapperType == wrapperType_Standalone) haveHost = false;
    if (! haveHost)   // standalone / no transport: free-run so it can still be heard
    {
        playing = true;
        ppq = freePpq;
        freePpq += buffer.getNumSamples() * bpm / (60.0 * getSampleRate());
    }

    uiBpm.store (bpm);
    uiSync.store (! playing ? 0 : (haveHost ? 1 : 2));
    engine.process (buffer, playing, bpm, ppq);
}

void HexProcessor::clearPattern()
{
    for (int i = 0; i < numSteps; ++i) engine.setStep (i, 0);
    activeEdited = true;
    sendChangeMessage();
}

void HexProcessor::loadSlot (int i)
{
    if (i < 0 || i >= numSlots) return;
    selectedSlot = i;
    for (int s = 0; s < numSteps; ++s) engine.setStep (s, slots[(size_t) i].steps[(size_t) s]);
    activeEdited = false;
    sendChangeMessage();
}

void HexProcessor::regenerate()
{
    const auto style = (Style) (int) apvts.getRawParameterValue ("style")->load();
    const float density = apvts.getRawParameterValue ("density")->load();

    juce::StringArray used;
    for (auto& s : slots) if (s.locked) used.add (s.name);

    for (auto& s : slots)
    {
        if (s.locked) continue;
        Pattern p;
        for (int tries = 0; tries < 12; ++tries)
        {
            p = PatternGenerator::make (style, density, rng);
            if (! used.contains (p.name)) break;
        }
        used.add (p.name);
        s = p;
    }

    if (! slots[(size_t) selectedSlot].locked || slots[(size_t) selectedSlot].isEmpty())
        loadSlot (selectedSlot);
    else
        sendChangeMessage();
}

void HexProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::ValueTree root ("HEXGATE");
    root.appendChild (apvts.copyState(), nullptr);
    juce::String active;
    for (int i = 0; i < numSteps; ++i) active << juce::String::charToString ((juce::juce_wchar) ('a' + engine.getStep (i)));
    root.setProperty ("active", active, nullptr);
    root.setProperty ("selected", selectedSlot, nullptr);
    for (auto& s : slots)
    {
        juce::ValueTree t ("Slot");
        t.setProperty ("steps", s.toString(), nullptr);
        t.setProperty ("name", s.name, nullptr);
        t.setProperty ("locked", s.locked, nullptr);
        root.appendChild (t, nullptr);
    }
    if (auto xml = root.createXml()) copyXmlToBinary (*xml, dest);
}

void HexProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr) return;
    auto root = juce::ValueTree::fromXml (*xml);
    if (! root.hasType ("HEXGATE")) return;
    auto params = root.getChildWithName (apvts.state.getType());
    if (params.isValid()) apvts.replaceState (params);
    int k = 0;
    for (auto c : root)
        if (c.hasType ("Slot") && k < numSlots)
        {
            auto& s = slots[(size_t) k++];
            s.fromString (c.getProperty ("steps").toString());
            s.name = c.getProperty ("name").toString();
            s.locked = c.getProperty ("locked", false);
        }
    selectedSlot = juce::jlimit (0, numSlots - 1, (int) root.getProperty ("selected", 0));
    Pattern a; a.fromString (root.getProperty ("active").toString());
    for (int i = 0; i < numSteps; ++i) engine.setStep (i, a.steps[(size_t) i]);
    sendChangeMessage();
}

juce::AudioProcessorEditor* HexProcessor::createEditor() { return new HexEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new HexProcessor(); }
