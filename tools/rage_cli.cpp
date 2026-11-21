// Offline test for FILIPINA RAGE: rage_cli <beat.wav> [outDir]
// Uses the beat as the sidechain and a synthetic sustained vocal as the input.
#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/rage/RageEngine.h"
#include <iostream>

using namespace rage;

int main (int argc, char** argv)
{
    if (argc < 2) { std::cout << "usage: rage_cli <beat> [outDir]\n"; return 1; }
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1])));
    if (! rd) return 1;
    const double sr = rd->sampleRate;
    const int n = (int) std::min<juce::int64> (rd->lengthInSamples, (juce::int64) (sr * 20));
    juce::AudioBuffer<float> beat (2, n);
    rd->read (&beat, 0, n, (juce::int64) (sr * 20), true, true); // 20s in

    // sustained "vocal": vibrato saw through formants, about -15 dBFS rms
    juce::AudioBuffer<float> vox (2, n);
    double ph = 0; float s1 = 0, s2 = 0;
    for (int i = 0; i < n; ++i)
    {
        const double t = i / sr;
        ph += 220.0 * (1 + 0.01 * std::sin (6.28 * 5 * t)) / sr; if (ph > 1) ph -= 1;
        const float src = (float) (2 * ph - 1);
        const float w = 2.0f * 3.14159f * 900.0f / (float) sr, r = 0.97f;
        const float y = src + 2 * r * std::cos (w) * s1 - r * r * s2; s2 = s1; s1 = y;
        vox.setSample (0, i, y); vox.setSample (1, i, y);
    }
    vox.applyGain (0.18f / vox.getRMSLevel (0, 0, n));

    juce::File out = argc >= 3 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]) : juce::File();
    if (out != juce::File()) out.createDirectory();

    // clean reference through the same path (mix 0)
    juce::AudioBuffer<float> ref (vox);
    {
        RageEngine e; e.prepare (sr, 512, 2);
        RageSettings s; s.mix = 0.0f; s.slam = 0.0f; e.setSettings (s);
        for (int pos = 0; pos < n; pos += 512)
        {
            const int len = std::min (512, n - pos);
            juce::AudioBuffer<float> blk (ref.getArrayOfWritePointers(), 2, pos, len);
            e.process (blk, beat.getReadPointer (0, pos), beat.getReadPointer (1, pos), len);
        }
    }

    for (int type = 0; type < (int) Crunch::NumCrunch; ++type)
        for (int mode = 0; mode < 2; ++mode)
        {
            RageEngine e; e.prepare (sr, 512, 2);
            RageSettings s; s.crunch = type; s.mode = mode; s.amount = 0.75f;
            e.setSettings (s);
            juce::AudioBuffer<float> buf (vox);
            std::vector<float> lev; double hitSq = 0, quietSq = 0; int hitN = 0, quietN = 0, bad = 0; float pk = 0;
            for (int pos = 0; pos < n; pos += 512)
            {
                const int len = std::min (512, n - pos);
                juce::AudioBuffer<float> blk (buf.getArrayOfWritePointers(), 2, pos, len);
                e.process (blk, beat.getReadPointer (0, pos), beat.getReadPointer (1, pos), len);
                const float L = e.hitLevel.load();
                for (int i = 0; i < len; ++i)
                {
                    const float v = buf.getSample (0, pos + i), d = ref.getSample (0, pos + i);
                    if (! std::isfinite (v)) ++bad;
                    pk = std::max (pk, std::fabs (v));
                    const float diff = v - d;
                    if (L > 0.5f) { hitSq += diff * diff; ++hitN; } else { quietSq += diff * diff; ++quietN; }
                }
            }
            auto db = [] (double sq, int k) { return k ? 10 * std::log10 (sq / k + 1e-12) : -999.0; };
            std::cout << crunchNames()[type].paddedRight (' ', 10) << (mode ? " follow" : " hit   ")
                      << "  hits " << e.hitCount.load() << "  change-during-hits " << juce::String (db (hitSq, hitN), 1)
                      << " dB  change-between " << juce::String (db (quietSq, quietN), 1) << " dB  peak " << juce::String (20 * std::log10 (pk), 1)
                      << (bad ? "  NaN!" : "") << "\n";
            if (out != juce::File() && mode == 0 && (type == 2 || type == 8))
            {
                buf.addFrom (0, 0, beat, 0, 0, n, 0.5f); buf.addFrom (1, 0, beat, 1, 0, n, 0.5f);
                juce::WavAudioFormat wav;
                auto f = out.getChildFile ("rage_" + crunchNames()[type] + ".wav"); f.deleteFile();
                std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
                w->writeFromAudioSampleBuffer (buf, 0, n);
            }
        }
    // learn
    RageEngine e; e.prepare (sr, 512, 2); e.startLearn();
    juce::AudioBuffer<float> buf (vox);
    for (int pos = 0; pos + 512 <= n && ! e.learnDone(); pos += 512)
    {
        juce::AudioBuffer<float> blk (buf.getArrayOfWritePointers(), 2, pos, 512);
        e.process (blk, beat.getReadPointer (0, pos), beat.getReadPointer (1, pos), 512);
    }
    std::cout << "learned 808 fundamental: " << e.learnResult() << " Hz\n";
    return 0;
}
