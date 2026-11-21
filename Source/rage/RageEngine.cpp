#include "RageEngine.h"

namespace rage
{

using namespace mime::dsp;

static inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }

static inline float kneeClip (float u, float knee, float ceiling)
{
    const float a = std::fabs (u);
    const float t = knee * ceiling;
    if (a <= t) return u;
    const float r = ceiling - t;
    const float y = t + r * std::tanh ((a - t) / r);
    return u < 0 ? -y : y;
}

void RageEngine::prepare (double sampleRate, int maxBlock, int numChannels)
{
    rate = sampleRate;
    numCh = juce::jlimit (1, 2, numChannels);
    os = std::make_unique<juce::dsp::Oversampling<float>> ((size_t) numCh, 2,
            juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    os->initProcessing ((size_t) maxBlock);
    osRate = (float) sampleRate * 4.0f;
    ctrl.assign ((size_t) maxBlock + 16, 0.0f);
    detBuf.assign ((size_t) maxBlock + 16, 0.0f);

    detEnv.set (0.0005f, 0.025f, (float) rate);
    detHP.set (25.0f, 0.707f, (float) rate);
    peakDecay = std::exp (-1.0f / (2.0f * (float) rate));

    for (int i = 0; i < (int) learnBands.size(); ++i)
    {
        const float f = 30.0f * std::pow (160.0f / 30.0f, (float) i / (float) (learnBands.size() - 1));
        learnBands[(size_t) i].set (f, 4.0f, (float) rate);
    }

    for (auto& c : ch)
    {
        c.dc.prepare (osRate);
        c.rumble.set (30.0f, 0.707f, osRate);
        c.tilt.set (1200.0f, 0.707f, osRate);
        c.megaHP.set (650.0f, 0.707f, osRate);
        c.megaLP.set (2800.0f, 0.707f, osRate);
        c.tapeLP.setLowpass (9000.0f, osRate);
        c.octHP.setLowpass (300.0f, osRate);
    }

    inCoef  = std::exp (-1.0f / (0.45f * osRate));
    preCoef = std::exp (-1.0f / (0.25f * osRate));
    agCoef  = std::exp (-1.0f / (0.08f * osRate));
    agSmooth = std::exp (-1.0f / (0.3f * osRate));
    reset();
}

void RageEngine::reset()
{
    for (auto& c : ch) c.reset();
    if (os) os->reset();
    detLP1.reset(); detLP2.reset(); detHP.reset(); detEnv.reset();
    runPeak = 1.0e-4f; armed = true; stage = 0; env = 0; followEnv = 0;
    inPow = 0.0156f; preGain = 1.0f; dryPow = wetPow = 1e-6f; agGain = 1.0f;
}

int RageEngine::getLatencySamples() const
{
    return os ? (int) std::round (os->getLatencyInSamples()) : 0;
}

void RageEngine::startLearn()
{
    learnEnergy.fill (0.0);
    learnSeconds = 0.0;
    for (auto& b : learnBands) b.reset();
    learnState.store (1);
}

void RageEngine::computeControl (const float* l, const float* r, int n)
{
    detLP1.set (set.detectHz, 0.707f, (float) rate);
    detLP2.set (set.detectHz, 0.707f, (float) rate);

    const float thr = dbToGain (-2.0f - 16.0f * set.sensitivity);
    const float attackInc = 1.0f / juce::jmax (1.0f, set.attackMs * 0.001f * (float) rate);
    const float holdSamples = set.holdMs * 0.001f * (float) rate;
    const float relCoef = std::exp (-3.0f / juce::jmax (1.0f, set.releaseMs * 0.001f * (float) rate));
    const float fAtt = std::exp (-1.0f / juce::jmax (1.0f, set.attackMs * 0.001f * (float) rate));
    const float fRel = relCoef;
    const bool learning = learnState.load() == 1;

    float maxEnv = 0.0f, maxRel = 0.0f, scPk = 0.0f;
    bool manual = manualHit.exchange (false);

    for (int i = 0; i < n; ++i)
    {
        float x = 0.0f;
        if (l != nullptr) x = r != nullptr ? 0.5f * (l[i] + r[i]) : l[i];
        scPk = juce::jmax (scPk, std::fabs (x));

        if (learning && std::fabs (x) > 0.003f)
        {
            for (size_t b = 0; b < learnBands.size(); ++b)
            {
                learnBands[b].tick (x);
                learnEnergy[b] += (double) learnBands[b].band * learnBands[b].band;
            }
            learnSeconds += 1.0 / rate;
        }

        detHP.tick (x);
        detLP1.tick (detHP.high);
        detLP2.tick (detLP1.low);
        detBuf[(size_t) i] = detLP2.low;
        const float e = detEnv.tick (detLP2.low);
        runPeak = juce::jmax (e, runPeak * peakDecay);
        const float rel = runPeak > 0.003f ? e / runPeak : 0.0f;
        maxRel = juce::jmax (maxRel, rel);

        // ---- hit trigger ----
        bool trig = false;
        if (armed && rel > thr) { trig = true; armed = false; }
        else if (! armed && rel < thr * 0.6f) armed = true;
        if (manual && i == 0) trig = true;

        if (set.mode == (int) Mode::Hit || manual || stage != 0)
        {
            if (trig && (set.mode == (int) Mode::Hit || manual))
            {
                stage = 1;
                hitCount.fetch_add (1);
            }
            switch (stage)
            {
                case 1: env += attackInc; if (env >= 1.0f) { env = 1.0f; stage = 2; holdLeft = holdSamples; } break;
                case 2: if ((holdLeft -= 1.0f) <= 0.0f) stage = 3; break;
                case 3: env *= relCoef; if (env < 0.0005f) { env = 0.0f; stage = 0; } break;
                default: break;
            }
        }

        float level = env;
        if (set.mode == (int) Mode::Follow)
        {
            if (trig) hitCount.fetch_add (1);
            const float target = juce::jlimit (0.0f, 1.0f, (rel - thr * 0.5f) / juce::jmax (0.05f, 1.0f - thr * 0.5f));
            followEnv = target + (target > followEnv ? fAtt : fRel) * (followEnv - target);
            level = juce::jmax (level, followEnv);
        }

        level = juce::jmax (level, set.idle);
        ctrl[(size_t) i] = level;
        maxEnv = juce::jmax (maxEnv, level);
    }

    if (learning && learnSeconds >= 6.0)
    {
        size_t best = 0;
        for (size_t b = 1; b < learnBands.size(); ++b) if (learnEnergy[b] > learnEnergy[best]) best = b;
        learnedHz.store (30.0f * std::pow (160.0f / 30.0f, (float) best / (float) (learnBands.size() - 1)));
        learnState.store (2);
    }

    hitLevel.store (maxEnv);
    detectLevel.store (maxRel);
    threshold.store (thr);
    scPeak.store (scPk);
}

float RageEngine::shape (int type, float x, int c, float d)
{
    auto& s = ch[(size_t) c];
    switch ((Crunch) type)
    {
        case Crunch::Tube:
        {
            const float g = dbToGain (d * 30.0f), bias = 0.22f;
            return (std::tanh (g * x + bias) - std::tanh (bias)) / std::sqrt (g);
        }
        case Crunch::Tape:
        {
            const float g = dbToGain (d * 24.0f);
            const float y = std::atan (g * x * 1.2f) * 0.8f / std::sqrt (g);
            return s.tapeLP.lp (y) * 0.35f + y * 0.65f;
        }
        case Crunch::Clip:
        {
            const float g = dbToGain (d * 34.0f), u = g * x;
            const float y = u >= 0 ? kneeClip (u, 0.75f, 0.8f) : kneeClip (u, 0.85f, 1.0f);
            return y / std::pow (g, 0.45f);
        }
        case Crunch::Fuzz:
        {
            const float g = dbToGain (d * 32.0f), a = std::tanh (g * x);
            return (0.7f * a + 0.45f * std::fabs (a)) / std::pow (g, 0.5f);
        }
        case Crunch::Fold:
        {
            const float g = 1.0f + d * 7.0f;
            return std::sin (g * x * 1.5707963f) / (1.5707963f * std::pow (g, 0.35f));
        }
        case Crunch::Crush:
        {
            const float pre = std::tanh (x * (1.0f + 2.5f * d));
            const float step = 2.0f / std::pow (2.0f, 12.0f - 9.0f * d);
            const int holdLen = 1 + (int) (d * d * 30.0f * osRate / 176400.0f);
            if (--s.holdCount <= 0) { s.holdCount = holdLen; s.held = std::round (pre / step) * step; }
            return s.held;
        }
        case Crunch::Octave:
        {
            const float g = dbToGain (d * 28.0f), a = std::tanh (g * x);
            const float oct = s.octHP.hp (std::fabs (a));
            return (0.55f * a + 1.1f * oct) / std::pow (g, 0.45f);
        }
        case Crunch::Megaphone:
        {
            s.megaHP.tick (x); s.megaLP.tick (s.megaHP.high);
            const float g = dbToGain (6.0f + d * 30.0f);
            return kneeClip (g * s.megaLP.low * 2.0f, 0.6f, 0.7f) / std::pow (g, 0.4f);
        }
        case Crunch::Destroy:
        {
            const float g = dbToGain (d * 36.0f);
            float y = kneeClip (g * x, 0.5f, 1.0f);
            const float step = 2.0f / std::pow (2.0f, 9.0f - 4.0f * d);
            const int holdLen = 1 + (int) (d * 10.0f * osRate / 176400.0f);
            if (--s.holdCount <= 0) { s.holdCount = holdLen; s.held = std::round (y / step) * step; }
            y = kneeClip (s.held * 1.6f, 0.6f, 0.9f);
            return y / std::pow (g, 0.35f);
        }
        default: break;
    }
    return x;
}

void RageEngine::process (juce::AudioBuffer<float>& main, const float* scL, const float* scR, int scSamples)
{
    const int n = main.getNumSamples();
    const int chans = juce::jmin (numCh, main.getNumChannels());
    if (n == 0 || chans == 0) return;
    if ((int) ctrl.size() < n) { ctrl.resize ((size_t) n); detBuf.resize ((size_t) n); }

    computeControl (scSamples >= n ? scL : nullptr, scSamples >= n ? scR : nullptr, n);

    // tuning aid: hear what the detector hears
    if (set.listenDetect)
    {
        for (int c = 0; c < chans; ++c)
        {
            auto* d = main.getWritePointer (c);
            for (int i = 0; i < n; ++i)
                d[i] = detBuf[(size_t) i] * (0.35f + 0.65f * ctrl[(size_t) i]) * 2.0f;
        }
        return;
    }

    if (set.crunch != lastCrunch) { for (auto& c : ch) { c.held = 0; c.holdCount = 0; } lastCrunch = set.crunch; }

    juce::dsp::AudioBlock<float> block (main.getArrayOfWritePointers(), (size_t) chans, (size_t) n);
    auto up = os->processSamplesUp (block);
    const int un = (int) up.getNumSamples();
    float* data[2] = { up.getChannelPointer (0), chans > 1 ? up.getChannelPointer (1) : nullptr };

    for (int c = 0; c < chans; ++c)
    {
        ch[(size_t) c].hp1.set (set.lowCut, 0.54f, osRate);
        ch[(size_t) c].hp2.set (set.lowCut, 1.31f, osRate);
    }
    const float tLow = dbToGain (-set.tone * 6.0f), tHigh = dbToGain (set.tone * 6.0f), tMid = std::sqrt (tLow * tHigh);
    const float outG = dbToGain (set.outputDb);

    for (int i = 0; i < un; ++i)
    {
        const float level = ctrl[(size_t) juce::jmin (n - 1, i >> 2)];
        const float drive = set.amount * level;
        const float m = set.mix * level;
        const float slam = dbToGain (set.slam * level);

        float mono = 0;
        for (int c = 0; c < chans; ++c) mono += data[c][i];
        mono /= (float) chans;
        inPow = mono * mono + inCoef * (inPow - mono * mono);
        if (inPow > 6.0e-6f)
        {
            const float want = juce::jlimit (0.25f, 8.0f, 0.12f / std::sqrt (inPow));
            preGain = want + preCoef * (preGain - want);
        }

        float dry[2] {}, wet[2] {};
        for (int c = 0; c < chans; ++c)
        {
            auto& s = ch[(size_t) c];
            s.hp1.tick (data[c][i]); s.hp2.tick (s.hp1.high);
            const float x = s.hp2.high;
            dry[c] = x;

            float w = shape (set.crunch, x * preGain, c, drive) / preGain;
            w = s.dc.tick (w);
            s.rumble.tick (w); w = s.rumble.high;
            s.tilt.tick (w);
            w = tLow * s.tilt.low + tMid * s.tilt.band + tHigh * s.tilt.high;
            wet[c] = w;
        }

        // auto gain: learn the wet/dry ratio while a hit is happening, hold it between hits
        float dp = 0, wp = 0;
        for (int c = 0; c < chans; ++c) { dp += dry[c] * dry[c]; wp += wet[c] * wet[c]; }
        if (level > 0.3f)
        {
            dryPow = dp + agCoef * (dryPow - dp);
            wetPow = wp + agCoef * (wetPow - wp);
            if (dryPow > 1e-8f)
            {
                const float want = juce::jlimit (0.1f, 3.0f, std::sqrt (dryPow / (wetPow + 1e-12f)));
                agGain = want + agSmooth * (agGain - want);
            }
        }
        const float ag = set.autoGain ? agGain : 1.0f;

        for (int c = 0; c < chans; ++c)
        {
            const float y = (dry[c] * (1.0f - m) + wet[c] * m * ag) * slam * outG;
            data[c][i] = std::isfinite (y) ? y : 0.0f;
        }
    }

    os->processSamplesDown (block);
}

} // namespace rage
