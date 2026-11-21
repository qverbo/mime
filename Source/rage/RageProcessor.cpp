#include "RageProcessor.h"
#include "RageEditor.h"

using namespace rage;

juce::AudioProcessorValueTreeState::ParameterLayout RageProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + "%"; });
    auto ms  = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return v < 10.0f ? String (v, 1) + " ms" : String (roundToInt (v)) + " ms"; });
    auto hz  = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v)) + " Hz"; });
    auto db  = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " dB"; });
    auto bip = AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return (v > 0 ? "+" : "") + String (roundToInt (v * 100.0f)); });
    auto skew = [] (float lo, float hi, float centre) { NormalisableRange<float> r (lo, hi); r.setSkewForCentre (centre); return r; };

    RageSettings d;
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "crunch", 1 }, "Crunch Type", crunchNames(), d.crunch));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "amount", 1 }, "Crunch Amount", NormalisableRange<float> (0, 1), d.amount, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "mix", 1 }, "Hit Mix", NormalisableRange<float> (0, 1), d.mix, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "slam", 1 }, "Slam", NormalisableRange<float> (0, 9), d.slam, db));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "idle", 1 }, "Idle Crunch", NormalisableRange<float> (0, 1), d.idle, pct));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "mode", 1 }, "Mode", StringArray { "Hit", "Follow" }, d.mode));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "attack", 1 }, "Attack", skew (0.1f, 50.0f, 5.0f), d.attackMs, ms));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "hold", 1 }, "Hold", skew (0.0f, 500.0f, 80.0f), d.holdMs, ms));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "release", 1 }, "Release", skew (10.0f, 2000.0f, 250.0f), d.releaseMs, ms));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "sens", 1 }, "Sensitivity", NormalisableRange<float> (0, 1), d.sensitivity, pct));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "detect", 1 }, "808 Freq", skew (40.0f, 250.0f, 100.0f), d.detectHz, hz));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "tone", 1 }, "Tone", NormalisableRange<float> (-1, 1), d.tone, bip));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "lowcut", 1 }, "Low Cut", skew (20.0f, 300.0f, 80.0f), d.lowCut, hz));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { "autogain", 1 }, "Auto Gain", d.autoGain));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { "output", 1 }, "Output", NormalisableRange<float> (-24, 12), d.outputDb, db));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { "listen", 1 }, "Hear 808 Detector", false));
    return { p.begin(), p.end() };
}

RageProcessor::RageProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    status = "Route your beat into the sidechain. Every 808 hit crunches the vocal. Hit TAP to test without a beat.";
    startTimerHz (15);
}

bool RageProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet(), out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (in != out) return false;
    if (layouts.inputBuses.size() > 1)
    {
        const auto sc = layouts.getChannelSet (true, 1);
        if (! (sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo())) return false;
    }
    return true;
}

void RageProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock, juce::jmax (1, getMainBusNumOutputChannels()));
    setLatencySamples (engine.getLatencySamples());
    histChunk = juce::jmax (64, (int) (sampleRate * 0.012));
}

RageSettings RageProcessor::readSettings() const
{
    auto v = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    RageSettings s;
    s.crunch = (int) v ("crunch");   s.amount = v ("amount");      s.mix = v ("mix");
    s.slam = v ("slam");             s.idle = v ("idle");          s.mode = (int) v ("mode");
    s.attackMs = v ("attack");       s.holdMs = v ("hold");        s.releaseMs = v ("release");
    s.sensitivity = v ("sens");      s.detectHz = v ("detect");    s.tone = v ("tone");
    s.lowCut = v ("lowcut");         s.autoGain = v ("autogain") > 0.5f;
    s.outputDb = v ("output");       s.listenDetect = v ("listen") > 0.5f;
    return s;
}

void RageProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();

    const float* scL = nullptr; const float* scR = nullptr; int scN = 0;
    if (getBusCount (true) > 1)
        if (auto* bus = getBus (true, 1); bus != nullptr && bus->isEnabled() && bus->getNumberOfChannels() > 0)
        {
            auto sc = getBusBuffer (buffer, true, 1);
            scL = sc.getReadPointer (0);
            scR = sc.getNumChannels() > 1 ? sc.getReadPointer (1) : nullptr;
            scN = sc.getNumSamples();
        }

    auto main = getBusBuffer (buffer, false, 0);
    engine.setSettings (readSettings());
    engine.process (main, scL, scR, scN);

    // scope history
    histLevelMax  = juce::jmax (histLevelMax, engine.hitLevel.load());
    histDetectMax = juce::jmax (histDetectMax, engine.detectLevel.load());
    histAccum += n;
    if (histAccum >= histChunk)
    {
        const int w = histWrite.load();
        histLevel[(size_t) w].store (histLevelMax);
        histDetect[(size_t) w].store (histDetectMax);
        histThreshold.store (engine.threshold.load());
        histWrite.store ((w + 1) % historySize);
        histAccum = 0; histLevelMax = 0; histDetectMax = 0;
    }
}

void RageProcessor::learn808()
{
    engine.startLearn();
    learning = true;
    setStatus ("Learning your 808... play the beat for about 6 seconds.");
}

void RageProcessor::timerCallback()
{
    if (learning && engine.learnDone())
    {
        learning = false;
        const float f = engine.learnResult();
        engine.clearLearn();
        const float detect = juce::jlimit (45.0f, 250.0f, f * 1.7f);
        if (auto* p = apvts.getParameter ("detect"))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (detect));
            p->endChangeGesture();
        }
        setStatus ("Your 808 sits around " + juce::String (juce::roundToInt (f)) + " Hz. Detector tuned to "
                   + juce::String (juce::roundToInt (detect)) + " Hz.");
    }
}

void RageProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void RageProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* RageProcessor::createEditor() { return new RageEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RageProcessor(); }
