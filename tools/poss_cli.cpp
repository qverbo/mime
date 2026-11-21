#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/poss/PossPresets.h"
#include <iostream>
using namespace poss;

int main (int argc, char** argv)
{
    const double sr = 44100; const int n = (int) (sr * 8);
    juce::AudioBuffer<float> vox (2, n), lead (2, n);
    double ph = 0; float s1 = 0, s2 = 0;
    for (int i = 0; i < n; ++i)
    {
        const double t = i / sr;
        ph += 247.0 * (1 + 0.005 * std::sin (6.28 * 5 * t)) / sr; if (ph > 1) ph -= 1;
        const float src = (float) (2 * ph - 1);
        const float w = 2.0f * 3.14159f * 1100.0f / (float) sr, rr = 0.97f;
        const float y = src + 2 * rr * std::cos (w) * s1 - rr * rr * s2; s2 = s1; s1 = y;
        vox.setSample (0, i, y); vox.setSample (1, i, y);
        const float g = std::fmod (t, 2.0) < 1.0 ? 1.0f : 0.0f;    // lead sings 1s on, 1s off
        lead.setSample (0, i, y * g); lead.setSample (1, i, y * g);
    }
    vox.applyGain (0.15f / vox.getRMSLevel (0, 0, n));
    lead.applyGain (0.15f / vox.getRMSLevel (0, 0, n) * 1.0f);

    mime::BeatProfile beat;
    if (argc > 1)
    {
        std::atomic<bool> c { false }; juce::String err;
        beat = mime::BeatAnalyzer::analyseFile (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]), c, err);
        std::cout << "beat: " << beat.summary() << "\n";
    }
    juce::File out = argc > 2 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]) : juce::File();
    if (out != juce::File()) out.createDirectory();

    auto rms = [] (const juce::AudioBuffer<float>& b, int from, int to) { double s = 0; for (int c = 0; c < 2; ++c) for (int i = from; i < to; ++i) s += b.getSample (c, i) * b.getSample (c, i); return juce::Decibels::gainToDecibels ((float) std::sqrt (s / (2.0 * (to - from)))); };
    auto side = [] (const juce::AudioBuffer<float>& b) { double m = 0, s = 0; for (int i = 0; i < b.getNumSamples(); ++i) { const float l = b.getSample (0, i), r = b.getSample (1, i); m += (l + r) * (l + r); s += (l - r) * (l - r); } return 10 * std::log10 (s / (m + 1e-12) + 1e-12); };

    std::cout << "dry rms " << rms (vox, 0, n) << "\n";
    PresetBank bank;
    for (int role = 0; role < (int) Role::NumRoles; ++role)
    {
        bank.generate (role, beat);
        std::cout << "\n== " << roleNames()[role] << "\n";
        for (int k = 0; k < PresetBank::numSlots; ++k)
        {
            const auto& p = bank.slots[(size_t) k];
            juce::AudioBuffer<float> a (vox), b (vox);
            PossEngine e1, e2; e1.prepare (sr, 512); e2.prepare (sr, 512);
            e1.setSettings (p.settings); e2.setSettings (p.settings);
            bool bad = false;
            for (int pos = 0; pos < n; pos += 512)
            {
                const int len = std::min (512, n - pos);
                juce::AudioBuffer<float> ba (a.getArrayOfWritePointers(), 2, pos, len), bb (b.getArrayOfWritePointers(), 2, pos, len);
                e1.process (ba, nullptr, nullptr, 0);
                e2.process (bb, lead.getReadPointer (0, pos), lead.getReadPointer (1, pos), len);
            }
            for (int i = 0; i < n; ++i) if (! std::isfinite (a.getSample (0, i))) bad = true;
            // ducking: compare layer level while lead sings (t in [4.3,4.9]) vs silent ([5.3,5.9])
            const float on = rms (b, (int) (4.3 * sr), (int) (4.9 * sr)), off = rms (b, (int) (5.3 * sr), (int) (5.9 * sr));
            std::cout << "  " << p.name.paddedRight (' ', 18) << p.lane.paddedRight (' ', 28)
                      << "rms " << juce::String (rms (a, 0, n), 1) << "  side/mid " << juce::String (side (a), 1)
                      << " dB  lead-on vs off " << juce::String (on - off, 1) << " dB" << (bad ? "  NaN!" : "") << "\n";
            if (out != juce::File() && k < 2)
            {
                juce::WavAudioFormat wav; auto f = out.getChildFile ("poss_" + roleNames()[role] + "_" + p.name.replaceCharacters (" /", "__") + ".wav"); f.deleteFile();
                std::unique_ptr<juce::AudioFormatWriter> wr (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
                wr->writeFromAudioSampleBuffer (a, 0, n);
            }
        }
    }
    bank.slots[0].locked = true;
    bank.generate (1, beat); std::cout << "\nregen w/ lock: " << bank.slots[0].name << " kept\n";
    bank.moreLikeThis (0, 1, beat);
    for (auto& s : bank.slots) std::cout << "  " << s.name << "\n";
    return 0;
}
