#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>
#include <deque>
#include "../MimeSettings.h"
#include "../analysis/BeatAnalyzer.h"

namespace mime
{

struct Preset
{
    juce::String name, lane, shortDesc, longDesc;
    Settings settings;
    int archetype = -1;
    bool locked = false;

    bool isEmpty() const { return archetype < 0; }
    juce::ValueTree toValueTree() const;
    static Preset fromValueTree (const juce::ValueTree&);
};

// Turns a BeatProfile into a handful of distinct, beat-aware presets.
class PresetGenerator
{
public:
    static constexpr int numSlots = 6;

    PresetGenerator();

    // Fresh set. Locked slots are kept.
    void generate (const BeatProfile& beat);

    // Fill every unlocked slot (except `index`) with variations of slot `index`.
    void moreLikeThis (int index, const BeatProfile& beat);

    std::array<Preset, numSlots> slots;

    juce::ValueTree toValueTree() const;
    void fromValueTree (const juce::ValueTree&);

    static int numArchetypes();

private:
    Preset build (int archetype, const BeatProfile& beat, const juce::String& avoidName);
    juce::Random rng;
    std::deque<int> recent;     // archetypes shown recently, so regenerate doesn't repeat itself
    std::deque<juce::String> recentNames;
};

} // namespace mime
