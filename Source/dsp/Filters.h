#pragma once

#include <cmath>
#include <vector>
#include <algorithm>

namespace mime::dsp
{

constexpr float kPi = 3.14159265358979f;

// Cytomic / Andy Simper trapezoidal state variable filter.
// low = v2, band (unity peak) = k * v1, high = v0 - k*v1 - v2.
struct SVF
{
    float g = 0, k = 1.41421356f, a1 = 0, a2 = 0, a3 = 0;
    float ic1 = 0, ic2 = 0;
    float low = 0, band = 0, high = 0;

    void set (float freq, float q, float sampleRate)
    {
        freq = std::clamp (freq, 10.0f, 0.49f * sampleRate);
        g  = std::tan (kPi * freq / sampleRate);
        k  = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    inline void tick (float v0)
    {
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        low  = v2;
        band = k * v1;
        high = v0 - k * v1 - v2;
    }

    void reset() { ic1 = ic2 = low = band = high = 0; }
};

// 4th order Linkwitz-Riley crossover built from two Butterworth SVF stages.
// low + high sums to an allpass (flat magnitude).
struct LR4
{
    SVF lp1, lp2, hp1, hp2;
    float low = 0, high = 0;

    void set (float freq, float sampleRate)
    {
        for (auto* f : { &lp1, &lp2, &hp1, &hp2 })
            f->set (freq, 0.70710678f, sampleRate);
    }

    inline void tick (float x)
    {
        lp1.tick (x);   lp2.tick (lp1.low);
        hp1.tick (x);   hp2.tick (hp1.high);
        low  = lp2.low;
        high = hp2.high;
    }

    // Allpass with the same phase as low+high at this crossover.
    inline float allpass (float x) { tick (x); return low + high; }

    void reset() { lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset(); low = high = 0; }
};

struct OnePole
{
    float a = 0, z = 0;
    void setLowpass (float freq, float sampleRate) { a = std::exp (-2.0f * kPi * freq / sampleRate); }
    void setTime (float seconds, float sampleRate) { a = std::exp (-1.0f / std::max (1.0e-6f, seconds * sampleRate)); }
    inline float lp (float x) { z = x + a * (z - x); return z; }
    inline float hp (float x) { return x - lp (x); }
    void reset() { z = 0; }
};

struct DCBlocker
{
    float r = 0.999f, x1 = 0, y1 = 0;
    void prepare (float sampleRate) { r = 1.0f - (2.0f * kPi * 12.0f / sampleRate); }
    inline float tick (float x) { const float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
    void reset() { x1 = y1 = 0; }
};

// Attack/release envelope follower on |x|.
struct Envelope
{
    float att = 0, rel = 0, env = 0;
    void set (float attackSec, float releaseSec, float sampleRate)
    {
        att = std::exp (-1.0f / (attackSec * sampleRate));
        rel = std::exp (-1.0f / (releaseSec * sampleRate));
    }
    inline float tick (float x)
    {
        x = std::fabs (x);
        const float c = x > env ? att : rel;
        env = x + c * (env - x);
        return env;
    }
    void reset() { env = 0; }
};

struct DelayLine
{
    std::vector<float> buf;
    int w = 0;
    void prepare (int maxSamples) { buf.assign ((size_t) std::max (4, maxSamples + 2), 0.0f); w = 0; }
    inline void push (float x) { buf[(size_t) w] = x; if (++w >= (int) buf.size()) w = 0; }
    inline float read (int delay) const
    {
        int r = w - 1 - delay;
        while (r < 0) r += (int) buf.size();
        return buf[(size_t) r];
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
};

// Schroeder allpass diffuser stage.
struct Allpass
{
    DelayLine d;
    int len = 1;
    float g = 0.6f;
    void prepare (float ms, float sampleRate)
    {
        len = std::max (1, (int) (ms * 0.001f * sampleRate));
        d.prepare (len + 1);
    }
    inline float tick (float x)
    {
        const float delayed = d.read (len - 1);
        const float y = delayed - g * x;
        d.push (x + g * y);
        return y;
    }
    void reset() { d.reset(); }
};

} // namespace mime::dsp
