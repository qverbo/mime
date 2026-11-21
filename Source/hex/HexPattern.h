#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <array>

namespace hex
{

enum class Fx : int
{
    Thru = 0, Stutter, Roll, Reverse, TapeStop, OctDown, OctUp, Gate, Scatter, NumFx
};

constexpr int numSteps = 16;

struct FxInfo { const char* code; const char* name; juce::uint32 colour; };

inline const FxInfo& fxInfo (Fx f)
{
    static const FxInfo table[] = {
        { "",   "THRU",      0xff16131c },
        { "ST", "STUTTER",   0xffa66bff },
        { "RL", "ROLL",      0xff6d3cff },
        { "RV", "REVERSE",   0xffe056fd },
        { "TS", "TAPE STOP", 0xff4b2a9e },
        { "-8", "OCT DOWN",  0xff8e7cc3 },
        { "+8", "OCT UP",    0xffeadcff },
        { "GT", "GATE",      0xff2b2236 },
        { "SC", "SCATTER",   0xffc39bd3 },
    };
    return table[juce::jlimit (0, (int) Fx::NumFx - 1, (int) f)];
}

struct Pattern
{
    std::array<int, numSteps> steps {};
    juce::String name;
    bool locked = false;

    bool isEmpty() const { for (auto s : steps) if (s != 0) return false; return true; }

    juce::String toString() const
    {
        juce::String s;
        for (auto v : steps) s << juce::String::charToString ((juce::juce_wchar) ('a' + v));
        return s;
    }
    void fromString (const juce::String& s)
    {
        for (int i = 0; i < numSteps; ++i)
            steps[(size_t) i] = i < s.length() ? juce::jlimit (0, (int) Fx::NumFx - 1, (int) (s[i] - 'a')) : 0;
    }
};

enum class Style : int { Glitch = 0, Haunt, Chop, Fill, Chaos, NumStyles };
inline juce::StringArray styleNames() { return { "GLITCH", "HAUNT", "CHOP", "FILL", "CHAOS" }; }

// Generates musically sensible glitch patterns for a style.
struct PatternGenerator
{
    static Pattern make (Style style, float density, juce::Random& rng);
    static juce::String makeName (juce::Random& rng);
};

} // namespace hex
