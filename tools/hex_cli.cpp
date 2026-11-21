#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/hex/HexEngine.h"
#include <iostream>
using namespace hex;

int main (int argc, char** argv)
{
    const double sr = 44100, bpm = 140;
    const int n = (int) (sr * 6.0);
    juce::AudioBuffer<float> vox (2, n);
    double ph = 0; float s1 = 0, s2 = 0;
    for (int i = 0; i < n; ++i)
    {
        const double t = i / sr;
        ph += 180.0 * (1 + 0.3 * std::sin (6.28 * 0.7 * t)) / sr; if (ph > 1) ph -= 1;
        const float src = (float) (2 * ph - 1);
        const float w = 2.0f * 3.14159f * 800.0f / (float) sr, r = 0.97f;
        const float y = src + 2 * r * std::cos (w) * s1 - r * r * s2; s2 = s1; s1 = y;
        const float env = (float) (0.5 + 0.5 * std::sin (6.28 * 1.3 * t));
        vox.setSample (0, i, y * env); vox.setSample (1, i, y * env);
    }
    vox.applyGain (0.2f / vox.getRMSLevel (0, 0, n));
    auto maxJump = [] (const juce::AudioBuffer<float>& b, int from)
    {
        float m = 0; const float* p = b.getReadPointer (0);
        for (int i = from + 1; i < b.getNumSamples(); ++i) m = std::max (m, std::fabs (p[i] - p[i - 1]));
        return m;
    };
    std::cout << "input max sample jump " << maxJump (vox, 0) << "\n";
    juce::File out = argc > 1 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]) : juce::File();
    if (out != juce::File()) out.createDirectory();

    for (int fx = 0; fx < (int) Fx::NumFx; ++fx)
    {
        HexEngine e; e.prepare (sr, 512, 2);
        HexSettings s; e.setSettings (s);
        for (int i = 0; i < numSteps; ++i) e.setStep (i, (i % 2 == 1 || i >= 12) ? fx : 0);
        juce::AudioBuffer<float> b (vox);
        double ppq = 0;
        for (int pos = 0; pos < n; pos += 512)
        {
            const int len = std::min (512, n - pos);
            juce::AudioBuffer<float> blk (b.getArrayOfWritePointers(), 2, pos, len);
            e.process (blk, true, bpm, ppq);
            ppq += len * bpm / (60.0 * sr);
        }
        bool bad = false; for (int i = 0; i < n; ++i) if (! std::isfinite (b.getSample (0, i))) bad = true;
        std::cout << fxInfo ((Fx) fx).name << "  max jump " << maxJump (b, (int) sr) << "  rms " << juce::Decibels::gainToDecibels (b.getRMSLevel (0, 0, n)) << (bad ? " NaN" : "") << "\n";
        if (out != juce::File())
        {
            juce::WavAudioFormat wav; auto f = out.getChildFile (juce::String ("hex_") + fxInfo ((Fx) fx).name + ".wav"); f.deleteFile();
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
            w->writeFromAudioSampleBuffer (b, 0, n);
        }
    }
    // generator sanity
    juce::Random rng (5);
    for (int st = 0; st < (int) Style::NumStyles; ++st)
        for (int k = 0; k < 3; ++k)
        {
            auto p = PatternGenerator::make ((Style) st, 0.5f, rng);
            std::cout << styleNames()[st].paddedRight (' ', 7) << p.name.paddedRight (' ', 14);
            for (auto v : p.steps) std::cout << (v ? fxInfo ((Fx) v).code : "..") << " ";
            std::cout << "\n";
        }
    return 0;
}
