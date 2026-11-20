#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>
#include "Filters.h"
#include "../MimeSettings.h"

namespace mime
{

// The MIME distortion engine.
//
// Signal flow (all at 4x oversampled rate):
//   in -> low cut -> 3-band split (LR4, phase-matched)
//      dry  = the untouched bands summed back together
//      wet  = low:  gentle warmth ("body")
//             mid:  main style (tube / tape / clip / fold / crush / fuzz)
//             high: texture  (sheen / crush / octave glass / fold)
//          -> DC + rumble clean-up -> focus (band-limit) -> tone tilt
//          -> fizz control (dynamic cut where harshness builds)
//          -> haze (diffuse ghost tail) -> width (mono-safe spread)
//   out = dry * (1 - mix) + wet * mix * autoGain  -> output gain
//
// The input level is tracked and normalised before the shapers, so a preset
// sounds roughly the same whether the vocal was recorded hot or quiet.
class MimeEngine
{
public:
    MimeEngine();

    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();

    // Call from the audio thread before process().
    void setSettings (const Settings& s) { target = s; }

    void process (juce::AudioBuffer<float>& buffer);

    int getLatencySamples() const;

    std::atomic<float> outputPeak { 0.0f };
    std::atomic<float> heat { 0.0f };   // how hard the shapers are working (0..1)

private:
    struct Channel
    {
        dsp::SVF lowCut1, lowCut2;
        dsp::LR4 xLow, xHigh, apLow;
        dsp::DCBlocker dc;
        dsp::SVF rumble;
        dsp::SVF focusHP1, focusHP2, focusLP1, focusLP2;
        dsp::SVF tilt;
        dsp::SVF fizzBand;
        dsp::Envelope fizzEnvBand, fizzEnvFull;
        dsp::OnePole tapeLP, octaveHP;

        // crush sample & hold state
        float midHeld = 0, highHeld = 0;
        int   midCount = 0, highCount = 0;

        // haze
        dsp::OnePole hazeInLP, hazeCombLP;
        std::array<dsp::Allpass, 4> hazeAP;
        dsp::DelayLine hazeComb;
        int hazeCombLen = 1;

        void reset();
    };

    float shapeMid (int style, float x, Channel& c, float drive);
    float shapeHigh (int texture, float x, Channel& c, float air);
    float processHaze (float x, Channel& c);
    void  updateCoefficients();

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    double baseRate = 44100.0;
    float  osRate = 176400.0f;
    int    numCh = 2;

    std::array<Channel, 2> ch;

    // width (cross-channel)
    dsp::DelayLine widthA, widthB;
    dsp::OnePole widthHP;
    int widthLenA = 1, widthLenB = 1;

    // level tracking
    float inPow = 0, preGain = 1.0f, preGainCoef = 0, inPowCoef = 0;
    float dryPow = 1.0e-6f, wetPow = 1.0e-6f, agCoef = 0, agGain = 1.0f, agSmooth = 0;

    Settings target, cur;
    bool first = true;

    // style crossfades
    int activeStyle = 0, prevStyle = 0, activeTexture = 0, prevTexture = 0;
    float styleFade = 1.0f, textureFade = 1.0f, fadeStep = 0.001f;

    float heatAcc = 0; int heatCount = 0;
};

} // namespace mime
