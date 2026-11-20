#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>

namespace mime
{

// Character of the main (mid band) distortion.
enum class Style : int
{
    Tube = 0,   // warm, asymmetric, even harmonics
    Tape,       // soft, rounded, glue
    Clip,       // blown-speaker hard clip
    Fold,       // wavefolder, metallic ring
    Crush,      // bit + sample-rate crush, digital dirt
    Fuzz,       // rectifier fuzz, buzzy
    NumStyles
};

// What happens to the top band (breath, S's, air).
enum class Texture : int
{
    Clean = 0,  // untouched
    Sheen,      // exciter-style soft saturation
    Crush,      // digital sparkle
    Octave,     // octave-up glass shimmer (rectifier)
    Fold,       // metallic fold
    NumTextures
};

inline juce::StringArray styleNames()   { return { "Tube", "Tape", "Clip", "Fold", "Crush", "Fuzz" }; }
inline juce::StringArray textureNames() { return { "Clean", "Sheen", "Crush", "Octave", "Fold" }; }

// Every value the engine needs. Presets are just Settings.
struct Settings
{
    int   style    = (int) Style::Tube;
    float drive    = 0.35f;   // 0..1   main grit amount
    int   texture  = (int) Texture::Sheen;
    float air      = 0.2f;    // 0..1   top-band texture amount
    float body     = 0.15f;   // 0..1   low-band warmth
    float xLow     = 300.0f;  // Hz     low/mid crossover
    float xHigh    = 4500.0f; // Hz     mid/high crossover
    float focus    = 0.0f;    // 0..1   0 = full range, 1 = tiny busted speaker
    float haze     = 0.0f;    // 0..1   diffuse ghost tail on the distortion
    float width    = 0.0f;    // 0..1   stereo spread of the distortion
    float fizz     = 0.4f;    // 0..1   dynamic harshness control
    float fizzFreq = 3500.0f; // Hz
    float tone     = 0.0f;    // -1..1  dark..bright tilt on the distortion
    float mix      = 0.5f;    // 0..1
    float lowCut   = 90.0f;   // Hz
    bool  autoGain = true;
    float outputDb = 0.0f;    // dB

    juce::ValueTree toValueTree (const juce::Identifier& type = "Settings") const
    {
        juce::ValueTree v (type);
        v.setProperty ("style", style, nullptr);
        v.setProperty ("drive", drive, nullptr);
        v.setProperty ("texture", texture, nullptr);
        v.setProperty ("air", air, nullptr);
        v.setProperty ("body", body, nullptr);
        v.setProperty ("xlow", xLow, nullptr);
        v.setProperty ("xhigh", xHigh, nullptr);
        v.setProperty ("focus", focus, nullptr);
        v.setProperty ("haze", haze, nullptr);
        v.setProperty ("width", width, nullptr);
        v.setProperty ("fizz", fizz, nullptr);
        v.setProperty ("fizzfreq", fizzFreq, nullptr);
        v.setProperty ("tone", tone, nullptr);
        v.setProperty ("mix", mix, nullptr);
        v.setProperty ("lowcut", lowCut, nullptr);
        v.setProperty ("autogain", autoGain, nullptr);
        v.setProperty ("output", outputDb, nullptr);
        return v;
    }

    static Settings fromValueTree (const juce::ValueTree& v)
    {
        Settings s;
        s.style    = v.getProperty ("style", s.style);
        s.drive    = v.getProperty ("drive", s.drive);
        s.texture  = v.getProperty ("texture", s.texture);
        s.air      = v.getProperty ("air", s.air);
        s.body     = v.getProperty ("body", s.body);
        s.xLow     = v.getProperty ("xlow", s.xLow);
        s.xHigh    = v.getProperty ("xhigh", s.xHigh);
        s.focus    = v.getProperty ("focus", s.focus);
        s.haze     = v.getProperty ("haze", s.haze);
        s.width    = v.getProperty ("width", s.width);
        s.fizz     = v.getProperty ("fizz", s.fizz);
        s.fizzFreq = v.getProperty ("fizzfreq", s.fizzFreq);
        s.tone     = v.getProperty ("tone", s.tone);
        s.mix      = v.getProperty ("mix", s.mix);
        s.lowCut   = v.getProperty ("lowcut", s.lowCut);
        s.autoGain = v.getProperty ("autogain", s.autoGain);
        s.outputDb = v.getProperty ("output", s.outputDb);
        s.sanitise();
        return s;
    }

    void sanitise()
    {
        style    = juce::jlimit (0, (int) Style::NumStyles - 1, style);
        texture  = juce::jlimit (0, (int) Texture::NumTextures - 1, texture);
        drive    = juce::jlimit (0.0f, 1.0f, drive);
        air      = juce::jlimit (0.0f, 1.0f, air);
        body     = juce::jlimit (0.0f, 1.0f, body);
        xLow     = juce::jlimit (120.0f, 800.0f, xLow);
        xHigh    = juce::jlimit (2000.0f, 9000.0f, xHigh);
        focus    = juce::jlimit (0.0f, 1.0f, focus);
        haze     = juce::jlimit (0.0f, 1.0f, haze);
        width    = juce::jlimit (0.0f, 1.0f, width);
        fizz     = juce::jlimit (0.0f, 1.0f, fizz);
        fizzFreq = juce::jlimit (1000.0f, 9000.0f, fizzFreq);
        tone     = juce::jlimit (-1.0f, 1.0f, tone);
        mix      = juce::jlimit (0.0f, 1.0f, mix);
        lowCut   = juce::jlimit (20.0f, 300.0f, lowCut);
        outputDb = juce::jlimit (-24.0f, 12.0f, outputDb);
    }
};

} // namespace mime
