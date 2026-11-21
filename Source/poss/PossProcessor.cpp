#include "PossProcessor.h"
#include "PossEditor.h"

using namespace poss;

namespace
{
    struct Def { const char* id; const char* name; float lo, hi, centre; float PossSettings::* field; int kind; };  // kind 0 pct, 1 hz, 2 ms, 3 db, 4 raw, 5 cents
    const std::vector<Def>& defs()
    {
        static const std::vector<Def> d = {
            { "dbl",         "Double",        0, 1, 0,     &PossSettings::dbl, 0 },
            { "detune",      "Double Detune", 0, 30, 0,    &PossSettings::detune, 5 },
            { "dblms",       "Double Delay",  5, 40, 0,    &PossSettings::dblMs, 2 },
            { "chorus",      "Chorus",        0, 1, 0,     &PossSettings::chorus, 0 },
            { "chorusrate",  "Chorus Rate",   0.05f, 4, 0.6f, &PossSettings::chorusRate, 1 },
            { "chorusdepth", "Chorus Depth",  0, 1, 0,     &PossSettings::chorusDepth, 0 },
            { "flange",      "Flange",        0, 1, 0,     &PossSettings::flange, 0 },
            { "flangerate",  "Flange Rate",   0.02f, 3, 0.3f, &PossSettings::flangeRate, 1 },
            { "flangefb",    "Flange Feedback", -0.9f, 0.9f, 0, &PossSettings::flangeFb, 4 },
            { "phase",       "Phase",         0, 1, 0,     &PossSettings::phase, 0 },
            { "phaserate",   "Phase Rate",    0.02f, 3, 0.3f, &PossSettings::phaseRate, 1 },
            { "phasefb",     "Phase Feedback", 0, 0.85f, 0, &PossSettings::phaseFb, 4 },
            { "wobble",      "Wobble",        0, 1, 0,     &PossSettings::wobble, 0 },
            { "wobblerate",  "Wobble Rate",   0.1f, 6, 1,  &PossSettings::wobbleRate, 1 },
            { "ghost",       "Ghost",         0, 1, 0,     &PossSettings::ghost, 0 },
            { "ghostsize",   "Ghost Size",    0, 1, 0,     &PossSettings::ghostSize, 0 },
            { "shimmer",     "Shimmer",       0, 1, 0,     &PossSettings::shimmer, 0 },
            { "dirt",        "Dirt",          0, 1, 0,     &PossSettings::dirt, 0 },
            { "lofi",        "Lo-Fi",         0, 1, 0,     &PossSettings::lofi, 0 },
            { "width",       "Width",         0, 1, 0,     &PossSettings::width, 0 },
            { "autopan",     "Autopan",       0, 1, 0,     &PossSettings::autopan, 0 },
            { "lowcut",      "Low Cut",       20, 600, 120, &PossSettings::lowCut, 1 },
            { "highcut",     "High Cut",      2000, 20000, 8000, &PossSettings::highCut, 1 },
            { "duck",        "Duck",          0, 12, 0,    &PossSettings::duck, 3 },
            { "bloom",       "Bloom",         0, 1, 0,     &PossSettings::bloom, 0 },
            { "mix",         "Mix",           0, 1, 0,     &PossSettings::mix, 0 },
            { "output",      "Output",        -24, 12, 0,  &PossSettings::outputDb, 3 },
        };
        return d;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout PossProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "role", 1 }, "Layer Role", roleNames(), 0));
    PossSettings dflt;
    for (auto& d : defs())
    {
        NormalisableRange<float> range (d.lo, d.hi);
        if (d.centre > 0) range.setSkewForCentre (d.centre);
        auto attr = AudioParameterFloatAttributes().withStringFromValueFunction ([kind = d.kind] (float v, int)
        {
            switch (kind)
            {
                case 0: return String (roundToInt (v * 100.0f)) + "%";
                case 1: return v >= 1000.0f ? String (v / 1000.0f, 1) + "k" : (v < 10.0f ? String (v, 2) + "Hz" : String (roundToInt (v)) + "Hz");
                case 2: return String (v, 1) + "ms";
                case 3: return String (v, 1) + "dB";
                case 5: return String (roundToInt (v)) + "ct";
                default: return String (v, 2);
            }
        });
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { d.id, 1 }, d.name, range, dflt.*(d.field), attr));
    }
    return { p.begin(), p.end() };
}

PossProcessor::PossProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",     juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output",    juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Lead Vocal", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    apvts.addParameterListener ("role", this);
    bank.generate (0, beat);
}

PossProcessor::~PossProcessor()
{
    apvts.removeParameterListener ("role", this);
    cancel = true;
    pool.removeAllJobs (true, 4000);
}

bool PossProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (in != out) return false;
    if (l.inputBuses.size() > 1)
    {
        const auto sc = l.getChannelSet (true, 1);
        if (! (sc.isDisabled() || sc == juce::AudioChannelSet::mono() || sc == juce::AudioChannelSet::stereo())) return false;
    }
    return true;
}

void PossProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) { engine.prepare (sampleRate, samplesPerBlock); }

PossSettings PossProcessor::readSettings() const
{
    PossSettings s;
    s.role = (int) apvts.getRawParameterValue ("role")->load();
    for (auto& d : defs()) s.*(d.field) = apvts.getRawParameterValue (d.id)->load();
    return s;
}

void PossProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
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
}

void PossProcessor::applyPreset (int i)
{
    if (i < 0 || i >= PresetBank::numSlots || bank.slots[(size_t) i].isEmpty()) return;
    const auto& s = bank.slots[(size_t) i].settings;
    applying = true;
    for (auto& d : defs())
    {
        if (std::string (d.id) == "mix" || std::string (d.id) == "output") continue;  // keep the user's mix/output
        if (auto* prm = apvts.getParameter (d.id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (s.*(d.field)));
            prm->endChangeGesture();
        }
    }
    applying = false;
    selected = i;
    sendChangeMessage();
}

void PossProcessor::regenerate()
{
    bank.generate (getRole(), beat);
    if (selected >= 0 && ! bank.slots[(size_t) selected].locked) applyPreset (selected);
    sendChangeMessage();
}

void PossProcessor::moreLikeThis()
{
    if (selected < 0) return;
    bank.moreLikeThis (selected, getRole(), beat);
    sendChangeMessage();
}

void PossProcessor::setRole (int role)
{
    if (auto* p = apvts.getParameter ("role"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) role));
        p->endChangeGesture();
    }
}

void PossProcessor::parameterChanged (const juce::String& id, float v)
{
    if (id != "role") return;
    const int r = juce::roundToInt (v);
    if (restoring.load() || r == lastRole.load()) { lastRole = r; return; }
    lastRole = r;
    if (! applying)
        juce::MessageManager::callAsync ([w = juce::WeakReference<PossProcessor> (this)]
        {
            if (auto* self = w.get()) { self->selected = -1; self->bank.generate (self->getRole(), self->beat); self->sendChangeMessage(); }
        });
}

void PossProcessor::analyseFile (const juce::File& f)
{
    if (analysing.load()) return;
    analysing = true; cancel = false;
    sendChangeMessage();
    juce::WeakReference<PossProcessor> weak (this);
    pool.addJob ([weak, f, this]
    {
        juce::String err;
        auto p = mime::BeatAnalyzer::analyseFile (f, cancel, err);
        juce::MessageManager::callAsync ([weak, p]
        {
            if (auto* self = weak.get())
            {
                self->analysing = false;
                if (p.valid) { self->beat = p; self->bank.generate (self->getRole(), p); self->selected = -1; }
                self->sendChangeMessage();
            }
        });
    });
}

void PossProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::ValueTree root ("POSSESSION");
    root.appendChild (apvts.copyState(), nullptr);
    root.appendChild (beat.toValueTree(), nullptr);
    root.appendChild (bank.toValueTree(), nullptr);
    root.setProperty ("selected", selected, nullptr);
    if (auto xml = root.createXml()) copyXmlToBinary (*xml, dest);
}

void PossProcessor::setStateInformation (const void* data, int size)
{
    auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr) return;
    auto root = juce::ValueTree::fromXml (*xml);
    if (! root.hasType ("POSSESSION")) return;
    restoring = true;
    auto params = root.getChildWithName (apvts.state.getType());
    if (params.isValid()) apvts.replaceState (params);
    lastRole = getRole();
    restoring = false;
    beat = mime::BeatProfile::fromValueTree (root.getChildWithName ("BeatProfile"));
    bank.fromValueTree (root.getChildWithName ("PossBank"));
    selected = root.getProperty ("selected", -1);
    sendChangeMessage();
}

juce::AudioProcessorEditor* PossProcessor::createEditor() { return new PossEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PossProcessor(); }
