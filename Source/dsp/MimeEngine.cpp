#include "MimeEngine.h"

namespace mime
{

using namespace dsp;

static inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }

static inline float softKneeClip (float u, float knee, float ceiling)
{
    const float a = std::fabs (u);
    const float t = knee * ceiling;
    if (a <= t) return u;
    const float r = ceiling - t;
    const float y = t + r * std::tanh ((a - t) / r);
    return u < 0 ? -y : y;
}

void MimeEngine::Channel::reset()
{
    for (auto* f : { &lowCut1, &lowCut2, &rumble, &focusHP1, &focusHP2, &focusLP1, &focusLP2, &tilt, &fizzBand })
        f->reset();
    xLow.reset(); xHigh.reset(); apLow.reset();
    dc.reset();
    fizzEnvBand.reset(); fizzEnvFull.reset();
    tapeLP.reset(); octaveHP.reset();
    midHeld = highHeld = 0; midCount = highCount = 0;
    hazeInLP.reset(); hazeCombLP.reset();
    for (auto& a : hazeAP) a.reset();
    hazeComb.reset();
}

MimeEngine::MimeEngine() {}

void MimeEngine::prepare (double sampleRate, int maxBlockSize, int numChannels)
{
    baseRate = sampleRate;
    numCh = juce::jlimit (1, 2, numChannels);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) numCh, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampler->initProcessing ((size_t) maxBlockSize);

    osRate = (float) (sampleRate * 4.0);

    const float hazeMs[2][4] = { { 4.1f, 6.7f, 11.3f, 17.9f }, { 4.7f, 7.9f, 12.6f, 19.4f } };
    const float combMs[2] = { 53.0f, 61.0f };

    for (int c = 0; c < 2; ++c)
    {
        auto& s = ch[(size_t) c];
        s.dc.prepare (osRate);
        s.rumble.set (28.0f, 0.707f, osRate);
        s.fizzEnvBand.set (0.0015f, 0.08f, osRate);
        s.fizzEnvFull.set (0.0015f, 0.08f, osRate);
        s.tapeLP.setLowpass (9000.0f, osRate);
        for (int i = 0; i < 4; ++i)
        {
            s.hazeAP[(size_t) i].prepare (hazeMs[c][i], osRate);
            s.hazeAP[(size_t) i].g = 0.62f;
        }
        s.hazeCombLen = (int) (combMs[c] * 0.001f * osRate);
        s.hazeComb.prepare (s.hazeCombLen + 2);
        s.hazeInLP.setLowpass (7000.0f, osRate);
        s.hazeCombLP.setLowpass (4500.0f, osRate);
        s.reset();
    }

    widthLenA = (int) (0.0071f * osRate);
    widthLenB = (int) (0.0113f * osRate);
    widthA.prepare (widthLenA + 2);
    widthB.prepare (widthLenB + 2);
    widthHP.setLowpass (280.0f, osRate);

    inPowCoef   = std::exp (-1.0f / (0.45f * osRate));
    preGainCoef = std::exp (-1.0f / (0.25f * osRate));
    agCoef      = std::exp (-1.0f / (0.6f * osRate));
    agSmooth    = std::exp (-1.0f / (0.15f * osRate));
    fadeStep    = 1.0f / (0.012f * osRate);

    reset();
}

void MimeEngine::reset()
{
    for (auto& c : ch) c.reset();
    widthA.reset(); widthB.reset(); widthHP.reset();
    if (oversampler) oversampler->reset();
    inPow = 0.0156f; preGain = 1.0f;
    dryPow = wetPow = 1.0e-6f; agGain = 1.0f;
    first = true;
}

int MimeEngine::getLatencySamples() const
{
    return oversampler ? (int) std::round (oversampler->getLatencyInSamples()) : 0;
}

void MimeEngine::updateCoefficients()
{
    const float focusHPf = cur.lowCut + std::pow (cur.focus, 1.4f) * 520.0f;
    const float focusLPf = 18000.0f * std::pow (3000.0f / 18000.0f, cur.focus);
    const float tiltG    = dbToGain (cur.tone * 6.0f);

    for (int c = 0; c < numCh; ++c)
    {
        auto& s = ch[(size_t) c];
        s.lowCut1.set (cur.lowCut, 0.54f, osRate);
        s.lowCut2.set (cur.lowCut, 1.31f, osRate);
        s.xLow.set (cur.xLow, osRate);
        s.xHigh.set (cur.xHigh, osRate);
        s.apLow.set (cur.xHigh, osRate);
        s.focusHP1.set (focusHPf, 0.707f, osRate);
        s.focusHP2.set (focusHPf, 0.707f, osRate);
        s.focusLP1.set (focusLPf, 0.707f, osRate);
        s.focusLP2.set (focusLPf, 0.707f, osRate);
        s.tilt.set (1200.0f, 0.707f, osRate);
        s.fizzBand.set (cur.fizzFreq, 1.3f, osRate);
        s.octaveHP.setLowpass (cur.xHigh, osRate);
        juce::ignoreUnused (tiltG);
    }
}

float MimeEngine::shapeMid (int style, float x, Channel& c, float drive)
{
    switch ((Style) style)
    {
        case Style::Tube:
        {
            const float g = dbToGain (drive * 30.0f);
            const float bias = 0.22f;
            return (std::tanh (g * x + bias) - std::tanh (bias)) / std::sqrt (g);
        }
        case Style::Tape:
        {
            const float g = dbToGain (drive * 24.0f);
            const float y = std::atan (g * x * 1.2f) * 0.8f / std::sqrt (g);
            return c.tapeLP.lp (y) * 0.35f + y * 0.65f;   // tape rounds off the edge
        }
        case Style::Clip:
        {
            const float g = dbToGain (drive * 32.0f);
            const float u = g * x;
            // asymmetric ceilings: the positive side breaks up first, like a pushed speaker
            const float y = u >= 0 ? softKneeClip (u, 0.75f, 0.8f) : softKneeClip (u, 0.85f, 1.0f);
            return y / std::pow (g, 0.45f);
        }
        case Style::Fold:
        {
            const float g = 1.0f + drive * 7.0f;
            return std::sin (g * x * 1.5707963f) / (1.5707963f * std::pow (g, 0.35f));
        }
        case Style::Crush:
        {
            const float pre = std::tanh (x * (1.0f + 2.5f * drive));
            const float bits = 12.0f - 9.0f * drive;
            const float step = 2.0f / std::pow (2.0f, bits);
            const int holdLen = 1 + (int) (drive * drive * 30.0f * osRate / 176400.0f);
            if (--c.midCount <= 0)
            {
                c.midCount = holdLen;
                c.midHeld = std::round (pre / step) * step;
            }
            return c.midHeld;
        }
        case Style::Fuzz:
        {
            const float g = dbToGain (drive * 30.0f);
            const float a = std::tanh (g * x);
            return (0.7f * a + 0.45f * std::fabs (a)) / std::pow (g, 0.5f);
        }
        default: break;
    }
    return x;
}

float MimeEngine::shapeHigh (int texture, float h, Channel& c, float air)
{
    switch ((Texture) texture)
    {
        case Texture::Clean:  return h;
        case Texture::Sheen:
        {
            const float g = 1.0f + 12.0f * air;
            return (1.0f - air) * h + air * std::tanh (g * h) * 0.28f;
        }
        case Texture::Crush:
        {
            const float bits = 10.0f - 6.0f * air;
            const float step = 2.0f / std::pow (2.0f, bits);
            const int holdLen = 1 + (int) (air * 7.0f * osRate / 176400.0f);
            if (--c.highCount <= 0)
            {
                c.highCount = holdLen;
                c.highHeld = std::round (std::tanh (h * 3.0f) / step) * step;
            }
            return (1.0f - 0.6f * air) * h + air * c.highHeld * 0.5f;
        }
        case Texture::Octave:
        {
            const float oct = c.octaveHP.hp (std::fabs (h) * 2.0f);
            return h + air * 1.4f * std::tanh (oct * 2.0f) * 0.5f;
        }
        case Texture::Fold:
        {
            const float g = 3.0f + 20.0f * air;
            return (1.0f - 0.7f * air) * h + air * std::sin (h * g) * 0.3f;
        }
        default: break;
    }
    return h;
}

float MimeEngine::processHaze (float x, Channel& c)
{
    float y = c.hazeInLP.lp (x);
    for (auto& ap : c.hazeAP)
        y = ap.tick (y);
    const float fb = 0.45f + 0.3f * cur.haze;
    const float tail = c.hazeComb.read (c.hazeCombLen - 1);
    c.hazeComb.push (y + fb * c.hazeCombLP.lp (tail));
    return tail * 0.8f + y * 0.35f;
}

void MimeEngine::process (juce::AudioBuffer<float>& buffer)
{
    const int channels = juce::jmin (numCh, buffer.getNumChannels());
    if (channels == 0 || buffer.getNumSamples() == 0) return;

    if (first) { cur = target; activeStyle = prevStyle = cur.style; activeTexture = prevTexture = cur.texture; first = false; }

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) channels, (size_t) buffer.getNumSamples());
    auto os = oversampler->processSamplesUp (block);
    const int n = (int) os.getNumSamples();

    float* data[2] = { os.getChannelPointer (0), channels > 1 ? os.getChannelPointer (1) : nullptr };

    float peak = 0.0f;
    constexpr int sub = 32;
    const float smooth = 0.03f;

    for (int start = 0; start < n; start += sub)
    {
        // parameter smoothing
        auto glide = [smooth] (float& a, float b) { a += (b - a) * smooth; };
        glide (cur.drive, target.drive);   glide (cur.air, target.air);     glide (cur.body, target.body);
        glide (cur.xLow, target.xLow);     glide (cur.xHigh, target.xHigh); glide (cur.focus, target.focus);
        glide (cur.haze, target.haze);     glide (cur.width, target.width); glide (cur.fizz, target.fizz);
        glide (cur.fizzFreq, target.fizzFreq); glide (cur.tone, target.tone); glide (cur.mix, target.mix);
        glide (cur.lowCut, target.lowCut); glide (cur.outputDb, target.outputDb);
        cur.autoGain = target.autoGain;

        if (target.style != activeStyle && styleFade >= 1.0f)
        { prevStyle = activeStyle; activeStyle = target.style; styleFade = 0.0f; }
        if (target.texture != activeTexture && textureFade >= 1.0f)
        { prevTexture = activeTexture; activeTexture = target.texture; textureFade = 0.0f; }

        updateCoefficients();

        const float tiltLow  = dbToGain (-cur.tone * 6.0f);
        const float tiltHigh = dbToGain ( cur.tone * 6.0f);
        const float tiltMid  = std::sqrt (tiltLow * tiltHigh);
        const float bodyG    = 1.0f + 4.0f * cur.body;
        const float fizzThr  = -14.0f + (1.0f - cur.fizz) * 10.0f;
        const float fizzMax  = cur.fizz * 12.0f;
        const float outG     = dbToGain (cur.outputDb);
        const int end = juce::jmin (n, start + sub);

        for (int i = start; i < end; ++i)
        {
            // ---- input level tracking (linked) ----
            float monoIn = 0.0f;
            for (int c = 0; c < channels; ++c) monoIn += data[c][i];
            monoIn /= (float) channels;
            inPow = monoIn * monoIn + inPowCoef * (inPow - monoIn * monoIn);
            const float inRms = std::sqrt (inPow);
            if (inRms > 0.0025f)
            {
                const float want = juce::jlimit (0.25f, 8.0f, 0.12f / inRms);
                preGain = want + preGainCoef * (preGain - want);
            }

            float dryOut[2] = {}, wetOut[2] = {};

            const float sFade = styleFade;
            const float tFade = textureFade;

            for (int c = 0; c < channels; ++c)
            {
                auto& s = ch[(size_t) c];
                float x = data[c][i];

                // low cut (24 dB/oct Butterworth)
                s.lowCut1.tick (x);  s.lowCut2.tick (s.lowCut1.high);
                x = s.lowCut2.high;

                // phase-matched 3-band split
                s.xLow.tick (x);
                const float low  = s.apLow.allpass (s.xLow.low);
                s.xHigh.tick (s.xLow.high);
                const float mid  = s.xHigh.low;
                const float high = s.xHigh.high;

                dryOut[c] = low + mid + high;

                // ---- wet ----
                const float pg = preGain, ipg = 1.0f / preGain;

                const float lw = std::tanh (bodyG * low * pg) / bodyG * ipg;

                float mw = shapeMid (activeStyle, mid * pg, s, cur.drive);
                if (sFade < 1.0f)
                    mw = mw * sFade + shapeMid (prevStyle, mid * pg, s, cur.drive) * (1.0f - sFade);
                mw *= ipg;

                float hw = shapeHigh (activeTexture, high * pg, s, cur.air);
                if (tFade < 1.0f)
                    hw = hw * tFade + shapeHigh (prevTexture, high * pg, s, cur.air) * (1.0f - tFade);
                hw *= ipg;

                float w = lw + mw + hw;

                // clean up DC / sub rumble created by asymmetric shapers
                w = s.dc.tick (w);
                s.rumble.tick (w);  w = s.rumble.high;

                // focus: band-limit the distortion (busted-speaker when high)
                if (cur.focus > 0.001f)
                {
                    s.focusHP1.tick (w); s.focusHP2.tick (s.focusHP1.high);
                    s.focusLP1.tick (s.focusHP2.high); s.focusLP2.tick (s.focusLP1.low);
                    w = s.focusLP2.low;
                }

                // tone tilt
                s.tilt.tick (w);
                w = tiltLow * s.tilt.low + tiltMid * s.tilt.band + tiltHigh * s.tilt.high;

                // fizz control: dynamic cut where harshness piles up
                s.fizzBand.tick (w);
                const float eb = s.fizzEnvBand.tick (s.fizzBand.band);
                const float ef = s.fizzEnvFull.tick (w);
                if (fizzMax > 0.01f)
                {
                    const float over = 20.0f * std::log10 ((eb + 1.0e-9f) / (ef + 1.0e-9f)) - fizzThr;
                    const float cut = juce::jmin (fizzMax, juce::jmax (0.0f, over) * 0.8f);
                    w -= (1.0f - dbToGain (-cut)) * s.fizzBand.band;
                }

                // haze: diffuse ghost tail built from the distortion
                if (cur.haze > 0.001f)
                    w = w * (1.0f - 0.45f * cur.haze) + processHaze (w, s) * cur.haze * 1.1f;

                wetOut[c] = w;
            }

            if (styleFade < 1.0f)   styleFade   = juce::jmin (1.0f, styleFade + fadeStep);
            if (textureFade < 1.0f) textureFade = juce::jmin (1.0f, textureFade + fadeStep);

            // width: mono-safe spread of the wet signal
            if (channels == 2 && cur.width > 0.001f)
            {
                const float m = 0.5f * (wetOut[0] + wetOut[1]);
                widthA.push (m); widthB.push (m);
                float side = widthA.read (widthLenA - 1) - widthB.read (widthLenB - 1);
                side = widthHP.hp (side) * cur.width * 0.55f;
                wetOut[0] += side;
                wetOut[1] -= side;
            }

            // auto gain: match the wet level to the dry level
            float dp = 0, wp = 0;
            for (int c = 0; c < channels; ++c) { dp += dryOut[c] * dryOut[c]; wp += wetOut[c] * wetOut[c]; }
            dryPow = dp + agCoef * (dryPow - dp);
            wetPow = wp + agCoef * (wetPow - wp);

            float ag = 1.0f;
            if (cur.autoGain)
            {
                if (dryPow > 1.0e-8f)
                {
                    const float want = juce::jlimit (0.06f, 3.0f, std::sqrt (dryPow / (wetPow + 1.0e-12f)));
                    agGain = want + agSmooth * (agGain - want);
                }
                ag = agGain;
            }

            for (int c = 0; c < channels; ++c)
            {
                const float y = (dryOut[c] * (1.0f - cur.mix) + wetOut[c] * cur.mix * ag) * outG;
                data[c][i] = std::isfinite (y) ? y : 0.0f;
                peak = juce::jmax (peak, std::fabs (data[c][i]));
            }
        }
    }

    oversampler->processSamplesDown (block);

    outputPeak.store (juce::jmax (peak, outputPeak.load() * 0.9f));
    heat.store (juce::jlimit (0.0f, 1.0f, cur.drive * cur.mix * 1.4f + cur.air * 0.2f));
}

} // namespace mime
