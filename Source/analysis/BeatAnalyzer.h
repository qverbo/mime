#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_data_structures/juce_data_structures.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>

namespace mime
{

// What MIME "heard" in the beat. All scores are 0..1.
struct BeatProfile
{
    bool  valid = false;
    float seconds = 0.0f;

    float grit = 0.4f;       // how distorted / squashed the beat already is
    float darkness = 0.5f;   // 1 = no top end, 0 = very bright
    float width = 0.4f;      // stereo width
    float space = 0.4f;      // washy / reverby / ambient vs punchy
    float ethereal = 0.4f;   // witch-house / ghostly character
    float subHeavy = 0.5f;   // how much of the energy is sub / 808
    float mud = 0.3f;        // low-mid build-up
    float busyFreq = 2800.0f;// where the beat crowds the vocal range (Hz)
    float crestDb = 12.0f;
    float centroidHz = 1500.0f;
    float rmsDb = -14.0f;

    juce::String vibe = "Not analysed yet";
    juce::StringArray tags;

    juce::String summary() const;   // one-line, plain English
    juce::ValueTree toValueTree() const;
    static BeatProfile fromValueTree (const juce::ValueTree&);
};

class BeatAnalyzer
{
public:
    void start (double sampleRate);
    void push (const float* left, const float* right, int numSamples);
    float secondsHeard() const { return (float) activeFrames * (float) hop / (float) rate; }
    BeatProfile finish() const;

    // Analyse a file on disk (call from a background thread).
    static BeatProfile analyseFile (const juce::File& file, std::atomic<bool>& cancel, juce::String& error);

private:
    void processFrame();

    static constexpr int order = 12;
    static constexpr int size = 1 << order;
    static constexpr int hop = size / 2;

    double rate = 44100.0;
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window, monoBuf, sideBuf, fftData, prevLogMag;
    std::vector<double> powerSum;
    int fill = 0;

    long long activeFrames = 0;
    double crestSum = 0, flatSum = 0, fluxSum = 0, midEnergy = 0, sideEnergy = 0, rmsSum = 0;
    long long fluxFrames = 0;
};

} // namespace mime
