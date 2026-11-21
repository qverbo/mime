#pragma once

#include <array>
#include <deque>
#include "PossEngine.h"
#include "../analysis/BeatAnalyzer.h"

namespace poss
{

struct Preset
{
    juce::String name, lane, desc;
    PossSettings settings;
    int archetype = -1;
    bool locked = false;
    bool isEmpty() const { return archetype < 0; }
    juce::ValueTree toValueTree() const;
    static Preset fromValueTree (const juce::ValueTree&);
};

class PresetBank
{
public:
    static constexpr int numSlots = 6;
    PresetBank();
    void generate (int role, const mime::BeatProfile& beat);
    void moreLikeThis (int index, int role, const mime::BeatProfile& beat);
    std::array<Preset, numSlots> slots;

    juce::ValueTree toValueTree() const;
    void fromValueTree (const juce::ValueTree&);

private:
    Preset build (int archetype, int role, const mime::BeatProfile& beat, const juce::String& avoid);
    juce::Random rng;
    std::deque<int> recent;
    std::deque<juce::String> recentNames;
};

} // namespace poss
