#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <array>
#include <vector>
#include "HexPattern.h"

namespace hex
{

struct HexSettings
{
    float mix = 1.0f;
    float chaos = 0.0f;
    int   grid = 1;          // 0 = 1/8, 1 = 1/16, 2 = 1/32
    int   repeats = 2;       // stutter repeats per step: index into {2,3,4,6,8}
    float smoothMs = 4.0f;
    float swing = 0.0f;      // 0..1
    float outputDb = 0.0f;
};

// Tempo-synced vocal glitcher. Records the input continuously and, on each
// step of the pattern, plays it back stuttered / reversed / tape-stopped / etc.
class HexEngine
{
public:
    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();

    void setSettings (const HexSettings& s) { set = s; }
    void setStep (int i, int fx) { pattern[(size_t) i].store (fx); }
    int  getStep (int i) const   { return pattern[(size_t) i].load(); }

    // playing=false passes audio through untouched.
    void process (juce::AudioBuffer<float>& buf, bool playing, double bpm, double ppqAtStart);

    std::atomic<int> currentStep { -1 };
    std::atomic<int> currentFx { 0 };
    std::atomic<float> stepPhase { 0.0f };

private:
    struct Voice
    {
        int fx = 0;
        juce::int64 capture = 0;
        double len = 1.0;
        double t = 0.0;
        int scatterSteps = 1;
    };

    float render (const Voice& v, int ch, float live) const;
    float readAt (int ch, double absPos) const;

    HexSettings set;
    std::array<std::atomic<int>, numSteps> pattern {};
    double rate = 44100.0;
    int numCh = 2;
    std::vector<std::vector<float>> ring;
    int ringSize = 1;
    juce::int64 writeCount = 0;

    Voice cur, prev;
    juce::int64 curGlobal = std::numeric_limits<juce::int64>::min();
    double fadeLeft = 0.0, fadeLen = 1.0;
    juce::Random rng;
    float outGain = 1.0f;
};

} // namespace hex
