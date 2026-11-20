#include "BeatAnalyzer.h"
#include <cmath>
#include <algorithm>

namespace mime
{

static float clamp01 (float x) { return juce::jlimit (0.0f, 1.0f, x); }

juce::String BeatProfile::summary() const
{
    if (! valid) return "MIME hasn't heard your beat yet. Hit LISTEN or drop the beat file here.";
    return vibe + "  -  " + tags.joinIntoString ("  /  ");
}

juce::ValueTree BeatProfile::toValueTree() const
{
    juce::ValueTree v ("BeatProfile");
    v.setProperty ("valid", valid, nullptr);
    v.setProperty ("seconds", seconds, nullptr);
    v.setProperty ("grit", grit, nullptr);
    v.setProperty ("darkness", darkness, nullptr);
    v.setProperty ("width", width, nullptr);
    v.setProperty ("space", space, nullptr);
    v.setProperty ("ethereal", ethereal, nullptr);
    v.setProperty ("subHeavy", subHeavy, nullptr);
    v.setProperty ("mud", mud, nullptr);
    v.setProperty ("busyFreq", busyFreq, nullptr);
    v.setProperty ("crestDb", crestDb, nullptr);
    v.setProperty ("centroidHz", centroidHz, nullptr);
    v.setProperty ("rmsDb", rmsDb, nullptr);
    v.setProperty ("vibe", vibe, nullptr);
    v.setProperty ("tags", tags.joinIntoString ("|"), nullptr);
    return v;
}

BeatProfile BeatProfile::fromValueTree (const juce::ValueTree& v)
{
    BeatProfile p;
    if (! v.isValid()) return p;
    p.valid      = v.getProperty ("valid", false);
    p.seconds    = v.getProperty ("seconds", 0.0f);
    p.grit       = v.getProperty ("grit", p.grit);
    p.darkness   = v.getProperty ("darkness", p.darkness);
    p.width      = v.getProperty ("width", p.width);
    p.space      = v.getProperty ("space", p.space);
    p.ethereal   = v.getProperty ("ethereal", p.ethereal);
    p.subHeavy   = v.getProperty ("subHeavy", p.subHeavy);
    p.mud        = v.getProperty ("mud", p.mud);
    p.busyFreq   = v.getProperty ("busyFreq", p.busyFreq);
    p.crestDb    = v.getProperty ("crestDb", p.crestDb);
    p.centroidHz = v.getProperty ("centroidHz", p.centroidHz);
    p.rmsDb      = v.getProperty ("rmsDb", p.rmsDb);
    p.vibe       = v.getProperty ("vibe", p.vibe).toString();
    p.tags       = juce::StringArray::fromTokens (v.getProperty ("tags").toString(), "|", "");
    p.tags.removeEmptyStrings();
    return p;
}

void BeatAnalyzer::start (double sampleRate)
{
    rate = sampleRate > 0 ? sampleRate : 44100.0;
    fft = std::make_unique<juce::dsp::FFT> (order);
    window.resize (size);
    for (int i = 0; i < size; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (size - 1));
    monoBuf.assign (size, 0.0f);
    sideBuf.assign (size, 0.0f);
    fftData.assign (size * 2, 0.0f);
    prevLogMag.assign (size / 2, 0.0f);
    powerSum.assign (size / 2, 0.0);
    fill = 0;
    activeFrames = fluxFrames = 0;
    crestSum = flatSum = fluxSum = midEnergy = sideEnergy = rmsSum = 0;
}

void BeatAnalyzer::push (const float* left, const float* right, int numSamples)
{
    if (! fft) start (rate);
    for (int i = 0; i < numSamples; ++i)
    {
        const float l = left[i];
        const float r = right != nullptr ? right[i] : l;
        monoBuf[(size_t) fill] = 0.5f * (l + r);
        sideBuf[(size_t) fill] = 0.5f * (l - r);
        if (++fill == size)
        {
            processFrame();
            std::copy (monoBuf.begin() + hop, monoBuf.end(), monoBuf.begin());
            std::copy (sideBuf.begin() + hop, sideBuf.end(), sideBuf.begin());
            fill = size - hop;
        }
    }
}

void BeatAnalyzer::processFrame()
{
    double sq = 0, sideSq = 0; float pk = 0;
    for (int i = 0; i < size; ++i)
    {
        const float m = monoBuf[(size_t) i];
        sq += (double) m * m;
        sideSq += (double) sideBuf[(size_t) i] * sideBuf[(size_t) i];
        pk = juce::jmax (pk, std::fabs (m));
    }
    const double rms = std::sqrt (sq / size);
    if (rms < 0.0056) // below about -45 dBFS: silence / tail, ignore
        return;

    ++activeFrames;
    rmsSum += 20.0 * std::log10 (rms);
    crestSum += 20.0 * std::log10 ((pk + 1.0e-9) / rms);
    midEnergy += sq;
    sideEnergy += sideSq;

    std::fill (fftData.begin(), fftData.end(), 0.0f);
    for (int i = 0; i < size; ++i)
        fftData[(size_t) i] = monoBuf[(size_t) i] * window[(size_t) i];
    fft->performFrequencyOnlyForwardTransform (fftData.data(), true);

    const double binHz = rate / size;
    double logSum = 0, linSum = 0; int flatBins = 0;
    double flux = 0; int fluxBins = 0;

    for (int b = 1; b < size / 2; ++b)
    {
        const double mag = fftData[(size_t) b];
        const double pw = mag * mag;
        powerSum[(size_t) b] += pw;

        const double f = b * binHz;
        if (f >= 1000.0 && f <= 8000.0)
        {
            logSum += std::log (pw + 1.0e-12);
            linSum += pw;
            ++flatBins;
        }
        const float lm = (float) std::log10 (mag + 1.0e-7);
        if (f >= 100.0 && f <= 8000.0)
        {
            flux += juce::jmax (0.0f, lm - prevLogMag[(size_t) b]);
            ++fluxBins;
        }
        prevLogMag[(size_t) b] = lm;
    }

    if (flatBins > 0 && linSum > 0)
        flatSum += std::exp (logSum / flatBins) / (linSum / flatBins);

    if (activeFrames > 1 && fluxBins > 0)
    {
        fluxSum += flux / fluxBins;
        ++fluxFrames;
    }
}

BeatProfile BeatAnalyzer::finish() const
{
    BeatProfile p;
    if (activeFrames < 8) return p;

    const double binHz = rate / size;
    auto bandPow = [&] (double lo, double hi)
    {
        double s = 0;
        for (int b = 1; b < size / 2; ++b)
        {
            const double f = b * binHz;
            if (f >= lo && f < hi) s += powerSum[(size_t) b];
        }
        return s;
    };

    const double total = bandPow (20.0, 20000.0) + 1.0e-20;
    auto relDb = [&] (double lo, double hi) { return (float) (10.0 * std::log10 (bandPow (lo, hi) / total + 1.0e-12)); };

    const float subDb  = relDb (20, 120);
    const float mudDb  = relDb (150, 500);
    const float airDb  = relDb (7000, 20000);

    double cNum = 0, cDen = 0;
    for (int b = 1; b < size / 2; ++b)
    {
        const double f = b * binHz;
        if (f < 40 || f > 16000) continue;
        cNum += f * powerSum[(size_t) b];
        cDen += powerSum[(size_t) b];
    }

    // where does the beat crowd the vocal? (1/3 octave scan, 1 - 5 kHz)
    float bestF = 2800.0f, bestDb = -1000.0f;
    for (double fc = 1000.0; fc <= 5000.0; fc *= std::pow (2.0, 1.0 / 6.0))
    {
        const float db = (float) (10.0 * std::log10 (bandPow (fc / std::pow (2.0, 1.0 / 6.0), fc * std::pow (2.0, 1.0 / 6.0)) + 1.0e-20))
                         + 1.5f * (float) std::log2 (fc / 1000.0); // gentle tilt: vocal clarity matters more up here
        if (db > bestDb) { bestDb = db; bestF = (float) fc; }
    }

    const float crest = (float) (crestSum / activeFrames);
    const float flat  = (float) (flatSum / activeFrames);
    const float flux  = fluxFrames > 0 ? (float) (fluxSum / fluxFrames) : 0.1f;
    const float sideDb = (float) (10.0 * std::log10 ((sideEnergy + 1.0e-20) / (midEnergy + 1.0e-20)));

    p.valid      = true;
    p.seconds    = secondsHeard();
    p.crestDb    = crest;
    p.centroidHz = (float) (cDen > 0 ? cNum / cDen : 1500.0);
    p.rmsDb      = (float) (rmsSum / activeFrames);
    p.busyFreq   = bestF;

    const float gritCrest = clamp01 ((17.0f - crest) / 8.0f);
    const float gritFlat  = clamp01 ((flat - 0.06f) / 0.22f);
    const float gritLoud  = clamp01 ((p.rmsDb + 20.0f) / 10.0f);
    p.grit      = clamp01 (0.45f * gritCrest + 0.35f * gritFlat + 0.2f * gritLoud);
    p.darkness  = clamp01 ((-10.0f - airDb) / 10.0f);
    p.width     = clamp01 ((sideDb + 20.0f) / 15.0f);
    p.subHeavy  = clamp01 ((subDb + 13.0f) / 10.0f);
    p.mud       = clamp01 ((mudDb + 16.0f) / 10.0f);
    p.space     = clamp01 (0.55f * clamp01 ((0.075f - flux) / 0.05f) + 0.45f * p.width);

    // vibe guess: score each lane, report a hybrid when two are close
    const float lowCentroid = clamp01 ((2200.0f - p.centroidHz) / 1400.0f);
    const float witch   = 0.4f * p.width + 0.25f * p.darkness + 0.15f * p.space + 0.2f * lowCentroid;
    const float rage    = 0.35f * p.grit + 0.15f * p.subHeavy + 0.3f * (1.0f - p.darkness) + 0.2f * (1.0f - p.space);
    const float darkUg  = 0.5f * p.darkness + 0.25f * p.subHeavy + 0.25f * (1.0f - p.width);
    const float melodic = 0.5f * (1.0f - p.grit) + 0.25f * (1.0f - p.darkness) + 0.25f * p.width;
    p.ethereal = clamp01 ((witch - 0.35f) / 0.4f);

    struct V { const char* name; float score; };
    std::vector<V> vs { { "Witch house", witch }, { "Rage", rage }, { "Dark underground", darkUg }, { "Melodic", melodic } };
    std::sort (vs.begin(), vs.end(), [] (const V& a, const V& b) { return a.score > b.score; });
    p.vibe = (vs[0].score - vs[1].score < 0.06f) ? juce::String (vs[0].name) + " meets " + juce::String (vs[1].name).toLowerCase()
                                                 : juce::String (vs[0].name);

    p.tags.add (p.grit > 0.6f ? "Already heavily distorted" : p.grit > 0.35f ? "Some grit already" : "Pretty clean");
    p.tags.add (p.darkness > 0.6f ? "Dark top end" : p.darkness < 0.3f ? "Bright top end" : "Balanced top end");
    if (p.subHeavy > 0.6f) p.tags.add ("Heavy sub");
    p.tags.add (p.width > 0.65f ? "Wide" : p.width < 0.3f ? "Narrow" : "Medium width");
    p.tags.add (p.space > 0.55f ? "Washy & spacey" : "Punchy");
    p.tags.add ("Crowded around " + juce::String (p.busyFreq / 1000.0f, 1) + " kHz");
    return p;
}

BeatProfile BeatAnalyzer::analyseFile (const juce::File& file, std::atomic<bool>& cancel, juce::String& error)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (file));
    if (reader == nullptr)
    {
        error = "Couldn't open that file. Try a WAV, AIFF, FLAC or MP3.";
        return {};
    }

    BeatAnalyzer a;
    a.start (reader->sampleRate);

    const juce::int64 maxSamples = std::min<juce::int64> (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 240.0));
    const int chunk = 16384;
    juce::AudioBuffer<float> buf (2, chunk);

    for (juce::int64 pos = 0; pos < maxSamples && ! cancel.load(); pos += chunk)
    {
        const int n = (int) std::min<juce::int64> ((juce::int64) chunk, maxSamples - pos);
        buf.clear();
        reader->read (&buf, 0, n, pos, true, true);
        a.push (buf.getReadPointer (0), reader->numChannels > 1 ? buf.getReadPointer (1) : nullptr, n);
    }

    auto p = a.finish();
    if (! p.valid) error = "That file seems to be silent or too short.";
    return p;
}

} // namespace mime
