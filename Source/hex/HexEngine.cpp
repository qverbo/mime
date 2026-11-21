#include "HexEngine.h"
#include <cmath>

namespace hex
{

void HexEngine::prepare (double sampleRate, int, int numChannels)
{
    rate = sampleRate;
    numCh = juce::jlimit (1, 2, numChannels);
    ringSize = (int) (sampleRate * 8.0);
    ring.assign ((size_t) numCh, std::vector<float> ((size_t) ringSize, 0.0f));
    reset();
}

void HexEngine::reset()
{
    for (auto& r : ring) std::fill (r.begin(), r.end(), 0.0f);
    writeCount = 0;
    cur = prev = Voice();
    curGlobal = std::numeric_limits<juce::int64>::min();
    fadeLeft = 0.0;
    currentStep = -1;
}

float HexEngine::readAt (int ch, double pos) const
{
    // never read the future, never read older than the ring holds
    const double newest = (double) (writeCount - 1);
    const double oldest = (double) (writeCount - ringSize + 4);
    pos = juce::jlimit (oldest, newest, pos);

    const auto i1 = (juce::int64) std::floor (pos);
    const float f = (float) (pos - (double) i1);
    auto at = [&] (juce::int64 i)
    {
        i = juce::jlimit (writeCount - ringSize + 1, writeCount - 1, i);
        return ring[(size_t) ch][(size_t) (i % ringSize)];
    };
    const float y0 = at (i1 - 1), y1 = at (i1), y2 = at (i1 + 1), y3 = at (i1 + 2);
    // 4-point Hermite
    const float c0 = y1, c1 = 0.5f * (y2 - y0);
    const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * f + c2) * f + c1) * f + c0;
}

float HexEngine::render (const Voice& v, int ch, float live) const
{
    const double L = v.len, t = v.t;
    const double cs = (double) v.capture;
    const double f = juce::jmax (1.0, set.smoothMs * 0.001 * rate);
    const double now = (double) (writeCount - 1);

    switch ((Fx) v.fx)
    {
        case Fx::Thru: return live;

        case Fx::Stutter:
        case Fx::Roll:
        {
            static const int reps[] = { 2, 3, 4, 6, 8 };
            const int r = reps[juce::jlimit (0, 4, set.repeats)] * (v.fx == (int) Fx::Roll ? 2 : 1);
            const double slice = L / r;
            const double k = std::floor (t / slice);
            const double ts = t - k * slice;
            double speed = 1.0;
            if (v.fx == (int) Fx::Roll && k >= 1) speed = 1.0 + 0.6 * juce::jmin (1.0, t / L); // rising pitch
            const double pos = juce::jmin (cs + ts * speed, now);
            double env = juce::jmin (1.0, (slice - ts) / f);
            if (k >= 1) env = juce::jmin (env, ts / f);
            return readAt (ch, pos) * (float) juce::jlimit (0.0, 1.0, env);
        }

        case Fx::Reverse:
            return readAt (ch, cs - 1.0 - t);

        case Fx::TapeStop:
        {
            const double u = juce::jmin (1.0, t / L), p = 1.6;
            const double pos = cs + L / (p + 1.0) * (1.0 - std::pow (1.0 - u, p + 1.0));
            const double env = juce::jlimit (0.0, 1.0, (1.0 - u) / 0.2);
            return readAt (ch, pos) * (float) env;
        }

        case Fx::OctDown:
            return readAt (ch, cs + 0.5 * t);

        case Fx::OctUp:
            return readAt (ch, juce::jmin (cs - L + 2.0 * t, now));

        case Fx::Gate:
            return 0.0f;

        case Fx::Scatter:
            return readAt (ch, cs - v.scatterSteps * L + t);

        default: break;
    }
    return live;
}

void HexEngine::process (juce::AudioBuffer<float>& buf, bool playing, double bpm, double ppq)
{
    const int n = buf.getNumSamples();
    const int chans = juce::jmin (numCh, buf.getNumChannels());
    if (n == 0 || chans == 0) return;

    outGain = juce::Decibels::decibelsToGain (set.outputDb);
    fadeLen = juce::jmax (1.0, set.smoothMs * 0.001 * rate);
    bpm = juce::jlimit (20.0, 400.0, bpm);

    static const double stepsPerBeatTable[] = { 2.0, 4.0, 8.0 };
    const double spb = stepsPerBeatTable[juce::jlimit (0, 2, set.grid)];
    const double unit = rate * 60.0 / (bpm * spb);          // samples per (unswung) step
    const double ppqPerSample = bpm / (60.0 * rate);
    const double sw = juce::jlimit (0.0, 1.0, (double) set.swing) * 0.33;

    float* d[2] = { buf.getWritePointer (0), chans > 1 ? buf.getWritePointer (1) : nullptr };

    for (int i = 0; i < n; ++i)
    {
        // record
        for (int c = 0; c < chans; ++c)
            ring[(size_t) c][(size_t) (writeCount % ringSize)] = d[c][i];
        ++writeCount;

        if (! playing)
        {
            curGlobal = std::numeric_limits<juce::int64>::min();
            currentStep = -1;
            fadeLeft = 0;
            continue;
        }

        // where are we in the grid? (with swing on every second step)
        const double sf = (ppq + i * ppqPerSample) * spb;
        const double pair = std::floor (sf / 2.0);
        const double pp = sf - 2.0 * pair;
        int local; double frac, lenUnits;
        if (pp < 1.0 + sw) { local = 0; lenUnits = 1.0 + sw; frac = pp / lenUnits; }
        else               { local = 1; lenUnits = 1.0 - sw; frac = (pp - 1.0 - sw) / lenUnits; }
        const auto global = (juce::int64) pair * 2 + local;
        const int idx = (int) (((global % numSteps) + numSteps) % numSteps);
        const double L = lenUnits * unit;

        if (global != curGlobal)
        {
            prev = cur;
            curGlobal = global;

            int fx = pattern[(size_t) idx].load();
            // chaos: surprise glitches on empty steps, swaps on busy ones
            if (set.chaos > 0.0f)
            {
                if (fx == 0 && rng.nextFloat() < set.chaos * 0.35f) fx = 1 + rng.nextInt ((int) Fx::NumFx - 1);
                else if (fx != 0 && rng.nextFloat() < set.chaos * 0.3f) fx = 1 + rng.nextInt ((int) Fx::NumFx - 1);
                if (fx == (int) Fx::Gate && rng.nextFloat() < 0.5f) fx = (int) Fx::Stutter;
            }

            cur.fx = fx;
            cur.capture = writeCount - 1;
            cur.len = L;
            cur.t = frac * L;
            cur.scatterSteps = 1 + rng.nextInt (4);
            fadeLeft = (prev.fx == 0 && cur.fx == 0) ? 0.0 : fadeLen;

            currentStep = idx;
            currentFx = fx;
        }
        else
        {
            cur.t = frac * L;
            cur.len = L;
        }
        stepPhase = (float) frac;

        for (int c = 0; c < chans; ++c)
        {
            const float live = d[c][i];
            float wet = render (cur, c, live);
            if (fadeLeft > 0.0)
            {
                const float a = (float) (1.0 - fadeLeft / fadeLen);
                const float g = std::sin (a * juce::MathConstants<float>::halfPi);
                const float old = render (prev, c, live);
                wet = wet * g + old * std::cos (a * juce::MathConstants<float>::halfPi);
            }
            const float y = (live * (1.0f - set.mix) + wet * set.mix) * outGain;
            d[c][i] = std::isfinite (y) ? y : 0.0f;
        }

        if (fadeLeft > 0.0) { fadeLeft -= 1.0; prev.t += 1.0; }
    }

    if (! playing)
        buf.applyGain (outGain);
}

} // namespace hex
