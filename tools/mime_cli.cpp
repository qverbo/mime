// Offline test tool for MIME's engine, analyser and preset generator.
//   mime_cli <beat.wav> [vocal.wav] [outDir]
// Analyses the beat, prints the profile, generates presets (plus a regenerate
// and a "more like this" round), and renders the vocal through every preset.
// With no vocal given, a synthetic sung vowel is used.

#include <juce_audio_formats/juce_audio_formats.h>
#include "../Source/dsp/MimeEngine.h"
#include "../Source/analysis/BeatAnalyzer.h"
#include "../Source/presets/PresetGenerator.h"
#include <iostream>
#include <map>

using namespace mime;

static juce::AudioBuffer<float> synthVocal (double sr, double seconds)
{
    const int n = (int) (sr * seconds);
    juce::AudioBuffer<float> b (2, n);
    juce::Random rnd (7);
    double ph = 0;
    const double formants[3] = { 700, 1220, 2600 };
    float st[3][2] = {};
    for (int i = 0; i < n; ++i)
    {
        const double t = i / sr;
        const double f0 = 196.0 * std::pow (2.0, (std::floor (t * 1.5) == 1 ? 3 : 0) / 12.0) * (1.0 + 0.012 * std::sin (2 * juce::MathConstants<double>::pi * 5.5 * t));
        ph += f0 / sr; if (ph >= 1) ph -= 1;
        double src = 2.0 * ph - 1.0 + 0.02 * (rnd.nextDouble() - 0.5);
        double out = 0;
        for (int k = 0; k < 3; ++k)
        {
            // crude 2-pole resonator
            const double w = 2 * juce::MathConstants<double>::pi * formants[k] / sr, r = 0.985;
            const double y = src + 2 * r * std::cos (w) * st[k][0] - r * r * st[k][1];
            st[k][1] = st[k][0]; st[k][0] = (float) y;
            out += y * (k == 0 ? 0.02 : k == 1 ? 0.012 : 0.008);
        }
        const double env = std::min (1.0, std::fmod (t, 0.66) * 30.0) * (std::fmod (t, 0.66) < 0.55 ? 1.0 : 0.0);
        const float s = (float) (out * env * 0.5);
        b.setSample (0, i, s); b.setSample (1, i, s);
    }
    const float rms = b.getRMSLevel (0, 0, n);
    b.applyGain (0.18f / (rms + 1e-9f)); // about -15 dBFS rms, a normal vocal level
    return b;
}

static void stats (const juce::String& label, const juce::AudioBuffer<float>& b)
{
    double sq = 0; float pk = 0; int bad = 0;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float v = b.getSample (c, i);
            if (! std::isfinite (v)) ++bad;
            sq += v * v; pk = std::max (pk, std::fabs (v));
        }
    const double rms = std::sqrt (sq / (b.getNumSamples() * b.getNumChannels()));
    std::cout << "  " << label.paddedRight (' ', 34) << " peak " << juce::String (20 * std::log10 (pk + 1e-9), 1)
              << " dB  rms " << juce::String (20 * std::log10 (rms + 1e-9), 1) << " dB" << (bad ? "  NaN/INF: " + juce::String (bad) : juce::String()) << "\n";
}

static void printPresets (const PresetGenerator& g)
{
    for (int i = 0; i < PresetGenerator::numSlots; ++i)
    {
        const auto& p = g.slots[(size_t) i];
        std::cout << "  [" << i << "] " << p.name << (p.locked ? " (locked)" : "") << "  -  lane: " << p.lane
                  << "  | " << styleNames()[p.settings.style] << "/" << textureNames()[p.settings.texture]
                  << " drive " << juce::String (p.settings.drive, 2) << " mix " << juce::String (p.settings.mix, 2)
                  << " haze " << juce::String (p.settings.haze, 2) << " width " << juce::String (p.settings.width, 2)
                  << " focus " << juce::String (p.settings.focus, 2) << " fizz@" << (int) p.settings.fizzFreq << "\n";
    }
}

int main (int argc, char** argv)
{
    if (argc < 2) { std::cout << "usage: mime_cli <beat> [vocal] [outDir]\n"; return 1; }
    juce::File beat (juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]));
    std::atomic<bool> cancel { false };
    juce::String err;
    auto prof = BeatAnalyzer::analyseFile (beat, cancel, err);
    if (! prof.valid) { std::cout << "analysis failed: " << err << "\n"; return 1; }

    std::cout << "BEAT: " << prof.summary() << "\n";
    std::cout << "  grit " << prof.grit << " dark " << prof.darkness << " width " << prof.width << " space " << prof.space << " ethereal " << prof.ethereal
              << " sub " << prof.subHeavy << " mud " << prof.mud << " busy " << prof.busyFreq << " crest " << prof.crestDb
              << " centroid " << prof.centroidHz << " rms " << prof.rmsDb << " secs " << prof.seconds << "\n\n";

    PresetGenerator gen;
    gen.generate (prof);
    std::cout << "PRESETS:\n"; printPresets (gen);
    std::cout << "\nLONG DESCRIPTION of [0]:\n" << gen.slots[0].longDesc << "\n\n";

    gen.slots[1].locked = true;
    gen.generate (prof);
    std::cout << "REGENERATE (slot 1 locked):\n"; printPresets (gen);
    gen.moreLikeThis (0, prof);
    std::cout << "\nMORE LIKE THIS (slot 0):\n"; printPresets (gen);

    // render
    const double sr = 44100.0;
    juce::AudioBuffer<float> vocal;
    if (argc >= 3 && juce::String (argv[2]) != "-")
    {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2])));
        if (rd) { vocal.setSize (2, (int) rd->lengthInSamples); rd->read (&vocal, 0, (int) rd->lengthInSamples, 0, true, true); }
    }
    if (vocal.getNumSamples() == 0) vocal = synthVocal (sr, 4.0);

    juce::File outDir = argc >= 4 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[3]) : juce::File();
    if (outDir != juce::File()) outDir.createDirectory();

    std::cout << "\nRENDER:\n";
    stats ("dry input", vocal);
    gen.generate (prof);
    for (int i = 0; i < PresetGenerator::numSlots; ++i)
    {
        MimeEngine eng;
        eng.prepare (sr, 512, 2);
        eng.setSettings (gen.slots[(size_t) i].settings);
        juce::AudioBuffer<float> out (vocal);
        for (int pos = 0; pos < out.getNumSamples(); pos += 512)
        {
            const int n = std::min (512, out.getNumSamples() - pos);
            juce::AudioBuffer<float> blk (out.getArrayOfWritePointers(), 2, pos, n);
            eng.process (blk);
        }
        stats (gen.slots[(size_t) i].name, out);
        if (outDir != juce::File())
        {
            juce::WavAudioFormat wav;
            auto f = outDir.getChildFile (juce::String (i) + "_" + gen.slots[(size_t) i].name.replaceCharacters (" /", "__") + ".wav");
            f.deleteFile();
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
            if (w) w->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
        }
    }

    // regen sweep: every archetype should appear and render cleanly
    {
        std::map<int, juce::String> seen;
        PresetGenerator sweep;
        for (int round = 0; round < 60; ++round)
        {
            sweep.generate (prof);
            for (auto& p : sweep.slots)
            {
                if (seen.count (p.archetype)) continue;
                seen[p.archetype] = p.name;
                MimeEngine eng; eng.prepare (sr, 512, 2); eng.setSettings (p.settings);
                juce::AudioBuffer<float> out (vocal);
                for (int pos = 0; pos < out.getNumSamples(); pos += 512)
                {
                    const int nn = std::min (512, out.getNumSamples() - pos);
                    juce::AudioBuffer<float> blk (out.getArrayOfWritePointers(), 2, pos, nn);
                    eng.process (blk);
                }
                stats (juce::String (p.archetype).paddedLeft ('0', 2) + " " + p.name + " (" + p.lane + ")", out);
            }
        }
        std::cout << "archetypes seen in 60 regens: " << seen.size() << " of " << PresetGenerator::numArchetypes() << "\n";
    }

    // every style x texture at max settings, mono + stereo, block size changes
    std::cout << "\nSTRESS:\n";
    for (int st = 0; st < (int) Style::NumStyles; ++st)
        for (int tx = 0; tx < (int) Texture::NumTextures; ++tx)
        {
            MimeEngine eng; eng.prepare (48000.0, 1024, (st + tx) % 2 ? 1 : 2);
            Settings s; s.style = st; s.texture = tx; s.drive = 1; s.air = 1; s.body = 1; s.haze = 1; s.width = 1; s.focus = 0.5f; s.mix = 1; s.fizz = 1;
            eng.setSettings (s);
            juce::AudioBuffer<float> b (vocal);
            b.applyGain (4.0f);
            int pos = 0, k = 0;
            const int sizes[] = { 1, 37, 256, 1024, 13 };
            while (pos < b.getNumSamples())
            {
                const int n = std::min (sizes[k++ % 5], b.getNumSamples() - pos);
                juce::AudioBuffer<float> blk (b.getArrayOfWritePointers(), (st + tx) % 2 ? 1 : 2, pos, n);
                if (pos > b.getNumSamples() / 2) { s.style = (st + 1) % (int) Style::NumStyles; eng.setSettings (s); }
                eng.process (blk);
                pos += n;
            }
            stats (styleNames()[st] + "/" + textureNames()[tx], b);
        }
    return 0;
}
