#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include <atomic>
#include <array>
#include <vector>
#include "../dsp/Filters.h"

namespace poss
{

enum class Role : int { Double = 0, Harmony, Adlib, Whisper, Low, NumRoles };
inline juce::StringArray roleNames() { return { "DOUBLE", "HARMONY", "AD-LIB", "WHISPER", "LOW" }; }

struct PossSettings
{
    int   role = 0;
    float dbl = 0.3f, detune = 12.0f, dblMs = 18.0f;
    float chorus = 0.0f, chorusRate = 0.6f, chorusDepth = 0.5f;
    float flange = 0.0f, flangeRate = 0.15f, flangeFb = 0.5f;
    float phase = 0.0f, phaseRate = 0.3f, phaseFb = 0.4f;
    float wobble = 0.0f, wobbleRate = 1.0f;
    float ghost = 0.0f, ghostSize = 0.5f, shimmer = 0.0f;
    float dirt = 0.0f, lofi = 0.0f;
    float width = 0.4f, autopan = 0.0f;
    float lowCut = 120.0f, highCut = 14000.0f;
    float duck = 3.0f, bloom = 0.3f;
    float mix = 1.0f, outputDb = 0.0f;

    juce::ValueTree toValueTree() const;
    static PossSettings fromValueTree (const juce::ValueTree&);
    void sanitise();
};

// Fractional delay line (Hermite).
struct ModDelay
{
    std::vector<float> buf; int w = 0, size = 1;
    void prepare (int maxSamples) { size = juce::jmax (8, maxSamples + 4); buf.assign ((size_t) size, 0.0f); w = 0; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    inline void push (float x) { buf[(size_t) w] = x; if (++w >= size) w = 0; }
    inline float tap (int d) const { int r = w - 1 - d; while (r < 0) r += size; return buf[(size_t) r]; }
    inline float read (float d) const
    {
        d = juce::jlimit (1.0f, (float) (size - 4), d);
        const int i = (int) d; const float f = d - (float) i;
        const float y0 = tap (i - 1), y1 = tap (i), y2 = tap (i + 1), y3 = tap (i + 2);
        const float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }
};

// Delay-based octave-up shifter (two crossfaded taps).
struct OctaveUp
{
    ModDelay d; float window = 2048, phase = 0;
    void prepare (float sr) { window = 0.05f * sr; d.prepare ((int) window + 8); phase = 0; }
    void reset() { d.reset(); phase = 0; }
    inline float tick (float x)
    {
        d.push (x);
        phase += 1.0f / window; if (phase >= 1.0f) phase -= 1.0f;
        float p2 = phase + 0.5f; if (p2 >= 1.0f) p2 -= 1.0f;
        const float a = window * (1.0f - phase) + 2.0f, b = window * (1.0f - p2) + 2.0f;
        const float wa = std::sin (juce::MathConstants<float>::pi * phase), wb = std::sin (juce::MathConstants<float>::pi * p2);
        return d.read (a) * wa * wa + d.read (b) * wb * wb;
    }
};

class PossEngine
{
public:
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void setSettings (const PossSettings& s) { set = s; }

    // main: 1 or 2 channels. sc: lead vocal (may be null).
    void process (juce::AudioBuffer<float>& main, const float* scL, const float* scR, int scSamples);

    std::atomic<float> leadActivity { 0.0f };   // 0..1
    std::atomic<bool>  leadPresent { false };
    std::atomic<float> intensity { 0.0f };      // how "possessed" the sound is, for the fog

private:
    PossSettings set;
    float sr = 44100.0f;

    struct Ch
    {
        mime::dsp::SVF hp, lp, lofiLP;
        ModDelay dblLine, chorusLine, flangeLine, wobbleLine, dryLine;
        std::array<float, 6> apState {};
        float phaseFbState = 0, flangeFbState = 0;
        float crushHeld = 0; int crushCount = 0;
        // ghost
        ModDelay pre, comb1, comb2;
        std::array<mime::dsp::Allpass, 4> diff;
        mime::dsp::OnePole damp1, damp2, ghostHP;
        OctaveUp oct;
        void reset();
    };
    std::array<Ch, 2> ch;
    ModDelay widthA, widthB;
    mime::dsp::OnePole widthHP;

    double lfo[8] {};   // phases
    float leadEnv = 0, leadPeak = 1e-4f, leadAct = 0, scSilentFor = 1e9f;
    float inPowA = 1e-6f, wetPowA = 1e-6f, levelG = 1.0f;
    int dryDelay = 1;
    juce::Random rng;
};

} // namespace poss
