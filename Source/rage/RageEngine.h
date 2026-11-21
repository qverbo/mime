#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include "../dsp/Filters.h"

namespace rage
{

enum class Crunch : int
{
    Tube = 0, Tape, Clip, Fuzz, Fold, Crush, Octave, Megaphone, Destroy, NumCrunch
};

inline juce::StringArray crunchNames()
{
    return { "Tube", "Tape", "Clip", "Fuzz", "Fold", "Crush", "Octave", "Megaphone", "Destroy" };
}

inline juce::StringArray crunchBlurbs()
{
    return {
        "Warm and thick. The vocal swells up on every hit.",
        "Soft, rounded squash. Subtle but you feel the 808 push the voice.",
        "Blown-speaker hard clip. Classic rage crunch.",
        "Buzzing fuzz-pedal grit. Heavy but smooth.",
        "Wave-folded metallic ring. Weird, robotic, futuristic.",
        "Bit-crushed digital dirt. Lo-fi and broken.",
        "Rectifier fuzz that adds an octave-up snarl on top.",
        "Squeezed through a busted speaker. No lows, all bark.",
        "Clip into crush into clip. Maximum damage.",
    };
}

enum class Mode : int { Hit = 0, Follow };

struct RageSettings
{
    int   crunch = (int) Crunch::Clip;
    float amount = 0.6f;      // how hard the crunch hits (0..1)
    float mix = 0.8f;         // wet blend at a full hit
    float slam = 2.0f;        // extra dB on hits
    float idle = 0.0f;        // crunch level between hits (0..1)
    int   mode = (int) Mode::Hit;
    float attackMs = 2.0f;
    float holdMs = 40.0f;
    float releaseMs = 180.0f;
    float sensitivity = 0.6f; // 0..1
    float detectHz = 110.0f;  // 808 detection low-pass
    float tone = 0.0f;        // -1..1
    float lowCut = 80.0f;
    bool  autoGain = true;
    float outputDb = 0.0f;
    bool  listenDetect = false;
};

// Sidechain-driven crunch: watches the 808 in the beat and distorts the vocal on every hit.
class RageEngine
{
public:
    void prepare (double sampleRate, int maxBlock, int numChannels);
    void reset();
    void setSettings (const RageSettings& s) { set = s; }

    // sc may be null (no sidechain). scChannels 1 or 2.
    void process (juce::AudioBuffer<float>& main, const float* scL, const float* scR, int scSamples);

    void triggerManualHit() { manualHit.store (true); }

    // learn the 808's fundamental
    void startLearn();
    bool learnDone() const { return learnState.load() == 2; }
    float learnResult() const { return learnedHz.load(); }
    void clearLearn() { learnState.store (0); }

    int getLatencySamples() const;

    // meters (audio -> UI)
    std::atomic<float> hitLevel { 0.0f };     // envelope 0..1 (block max)
    std::atomic<float> detectLevel { 0.0f };  // detect env relative to running peak (0..1+)
    std::atomic<float> threshold { 0.5f };    // threshold relative to running peak
    std::atomic<int>   hitCount { 0 };
    std::atomic<float> scPeak { 0.0f };

private:
    float shape (int type, float x, int ch, float drive);
    void  computeControl (const float* l, const float* r, int n);

    RageSettings set;
    double rate = 44100.0;
    float osRate = 176400.0f;
    int numCh = 2;
    std::unique_ptr<juce::dsp::Oversampling<float>> os;

    // detection (base rate)
    mime::dsp::SVF detLP1, detLP2, detHP;
    mime::dsp::Envelope detEnv;
    float runPeak = 1.0e-4f, peakDecay = 0.9999f;
    bool armed = true;
    int stage = 0;           // 0 idle, 1 attack, 2 hold, 3 release
    float env = 0.0f, holdLeft = 0.0f;
    float followEnv = 0.0f;
    std::vector<float> ctrl, detBuf;
    std::atomic<bool> manualHit { false };

    // learn
    std::atomic<int> learnState { 0 };   // 0 idle, 1 learning, 2 done
    std::atomic<float> learnedHz { 0.0f };
    std::array<mime::dsp::SVF, 12> learnBands;
    std::array<double, 12> learnEnergy {};
    double learnSeconds = 0.0;

    // per channel (oversampled)
    struct Ch
    {
        mime::dsp::SVF hp1, hp2, rumble, tilt, megaHP, megaLP;
        mime::dsp::DCBlocker dc;
        mime::dsp::OnePole tapeLP, octHP;
        float held = 0; int holdCount = 0;
        void reset() { hp1.reset(); hp2.reset(); rumble.reset(); tilt.reset(); megaHP.reset(); megaLP.reset(); dc.reset(); tapeLP.reset(); octHP.reset(); held = 0; holdCount = 0; }
    };
    std::array<Ch, 2> ch;

    float inPow = 0.0156f, preGain = 1.0f, inCoef = 0, preCoef = 0;
    float dryPow = 1e-6f, wetPow = 1e-6f, agCoef = 0, agSmooth = 0, agGain = 1.0f;
    int lastCrunch = -1;
};

} // namespace rage
