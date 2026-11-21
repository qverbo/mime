#include "PossEngine.h"
#include <cmath>

namespace poss
{

using namespace mime::dsp;
static constexpr float twoPi = 6.2831853f;

// ------------------------------------------------------------ settings io

juce::ValueTree PossSettings::toValueTree() const
{
    juce::ValueTree v ("Poss");
    auto p = [&v] (const char* k, float x) { v.setProperty (k, x, nullptr); };
    p ("role", (float) role); p ("dbl", dbl); p ("detune", detune); p ("dblMs", dblMs);
    p ("chorus", chorus); p ("chorusRate", chorusRate); p ("chorusDepth", chorusDepth);
    p ("flange", flange); p ("flangeRate", flangeRate); p ("flangeFb", flangeFb);
    p ("phase", phase); p ("phaseRate", phaseRate); p ("phaseFb", phaseFb);
    p ("wobble", wobble); p ("wobbleRate", wobbleRate);
    p ("ghost", ghost); p ("ghostSize", ghostSize); p ("shimmer", shimmer);
    p ("dirt", dirt); p ("lofi", lofi); p ("width", width); p ("autopan", autopan);
    p ("lowCut", lowCut); p ("highCut", highCut); p ("duck", duck); p ("bloom", bloom);
    p ("mix", mix); p ("outputDb", outputDb);
    return v;
}

PossSettings PossSettings::fromValueTree (const juce::ValueTree& v)
{
    PossSettings s;
    auto g = [&v] (const char* k, float def) { return (float) v.getProperty (k, def); };
    s.role = (int) g ("role", 0); s.dbl = g ("dbl", s.dbl); s.detune = g ("detune", s.detune); s.dblMs = g ("dblMs", s.dblMs);
    s.chorus = g ("chorus", s.chorus); s.chorusRate = g ("chorusRate", s.chorusRate); s.chorusDepth = g ("chorusDepth", s.chorusDepth);
    s.flange = g ("flange", s.flange); s.flangeRate = g ("flangeRate", s.flangeRate); s.flangeFb = g ("flangeFb", s.flangeFb);
    s.phase = g ("phase", s.phase); s.phaseRate = g ("phaseRate", s.phaseRate); s.phaseFb = g ("phaseFb", s.phaseFb);
    s.wobble = g ("wobble", s.wobble); s.wobbleRate = g ("wobbleRate", s.wobbleRate);
    s.ghost = g ("ghost", s.ghost); s.ghostSize = g ("ghostSize", s.ghostSize); s.shimmer = g ("shimmer", s.shimmer);
    s.dirt = g ("dirt", s.dirt); s.lofi = g ("lofi", s.lofi); s.width = g ("width", s.width); s.autopan = g ("autopan", s.autopan);
    s.lowCut = g ("lowCut", s.lowCut); s.highCut = g ("highCut", s.highCut); s.duck = g ("duck", s.duck); s.bloom = g ("bloom", s.bloom);
    s.mix = g ("mix", s.mix); s.outputDb = g ("outputDb", s.outputDb);
    s.sanitise();
    return s;
}

void PossSettings::sanitise()
{
    auto c01 = [] (float& x) { x = juce::jlimit (0.0f, 1.0f, x); };
    role = juce::jlimit (0, (int) Role::NumRoles - 1, role);
    for (auto* x : { &dbl, &chorus, &chorusDepth, &flange, &phase, &wobble, &ghost, &ghostSize, &shimmer, &dirt, &lofi, &width, &autopan, &bloom, &mix })
        c01 (*x);
    detune = juce::jlimit (0.0f, 30.0f, detune);
    dblMs = juce::jlimit (5.0f, 40.0f, dblMs);
    chorusRate = juce::jlimit (0.05f, 4.0f, chorusRate);
    flangeRate = juce::jlimit (0.02f, 3.0f, flangeRate);
    flangeFb = juce::jlimit (-0.9f, 0.9f, flangeFb);
    phaseRate = juce::jlimit (0.02f, 3.0f, phaseRate);
    phaseFb = juce::jlimit (0.0f, 0.85f, phaseFb);
    wobbleRate = juce::jlimit (0.1f, 6.0f, wobbleRate);
    lowCut = juce::jlimit (20.0f, 600.0f, lowCut);
    highCut = juce::jlimit (2000.0f, 20000.0f, highCut);
    duck = juce::jlimit (0.0f, 12.0f, duck);
    outputDb = juce::jlimit (-24.0f, 12.0f, outputDb);
}

// ------------------------------------------------------------ engine

void PossEngine::Ch::reset()
{
    hp.reset(); lp.reset(); lofiLP.reset();
    for (auto* d : { &dblLine, &chorusLine, &flangeLine, &wobbleLine, &dryLine, &pre, &comb1, &comb2 }) d->reset();
    apState.fill (0.0f); phaseFbState = flangeFbState = 0; crushHeld = 0; crushCount = 0;
    for (auto& a : diff) a.reset();
    damp1.reset(); damp2.reset(); ghostHP.reset(); oct.reset();
}

void PossEngine::prepare (double sampleRate, int)
{
    sr = (float) sampleRate;
    const float diffMs[2][4] = { { 4.7f, 7.1f, 11.9f, 16.3f }, { 5.3f, 8.3f, 12.7f, 17.9f } };
    for (int c = 0; c < 2; ++c)
    {
        auto& s = ch[(size_t) c];
        s.dblLine.prepare ((int) (0.08f * sr));
        s.chorusLine.prepare ((int) (0.06f * sr));
        s.flangeLine.prepare ((int) (0.02f * sr));
        s.wobbleLine.prepare ((int) (0.03f * sr));
        s.dryLine.prepare ((int) (0.01f * sr));
        s.pre.prepare ((int) (0.05f * sr));
        s.comb1.prepare ((int) (0.1f * sr));
        s.comb2.prepare ((int) (0.1f * sr));
        for (int i = 0; i < 4; ++i) { s.diff[(size_t) i].prepare (diffMs[c][i], sr); s.diff[(size_t) i].g = 0.65f; }
        s.damp1.setLowpass (5000.0f, sr);
        s.damp2.setLowpass (4200.0f, sr);
        s.ghostHP.setLowpass (250.0f, sr);
        s.oct.prepare (sr);
    }
    widthA.prepare ((int) (0.02f * sr));
    widthB.prepare ((int) (0.02f * sr));
    widthHP.setLowpass (300.0f, sr);
    dryDelay = (int) (0.005f * sr);
    reset();
}

void PossEngine::reset()
{
    for (auto& c : ch) c.reset();
    widthA.reset(); widthB.reset(); widthHP.reset();
    for (auto& p : lfo) p = 0.0;
    lfo[1] = 1.3; lfo[2] = 2.1; lfo[3] = 0.7; lfo[6] = 0.4;
    leadEnv = 0; leadPeak = 1e-4f; leadAct = 0; scSilentFor = 1e9f; inPowA = wetPowA = 1e-6f; levelG = 1.0f;
}

void PossEngine::process (juce::AudioBuffer<float>& main, const float* scL, const float* scR, int scN)
{
    const int n = main.getNumSamples();
    const int chans = juce::jmin (2, main.getNumChannels());
    if (n == 0 || chans == 0) return;
    const bool haveSc = scL != nullptr && scN >= n;

    // per-block coefficients
    const float lofiHz = 18000.0f * std::pow (3200.0f / 18000.0f, set.lofi);
    for (auto& c : ch)
    {
        c.hp.set (set.lowCut, 0.707f, sr);
        c.lp.set (set.highCut, 0.707f, sr);
        c.lofiLP.set (lofiHz, 0.707f, sr);
    }

    const float dirtG = 1.0f + set.dirt * 8.0f;
    const float dirtComp = 1.0f / std::tanh (juce::jmax (0.1f, dirtG * 0.35f)) * 0.35f;
    const float dblRate = 0.45f;
    const float dblDepth = set.detune / 1731.0f * sr / (twoPi * dblRate);
    const float dblBase = set.dblMs * 0.001f * sr + dblDepth + 2.0f;
    const float chBase[3] = { 0.012f * sr, 0.017f * sr, 0.023f * sr };
    const float chDepth = (0.5f + 3.0f * set.chorusDepth) * 0.001f * sr;
    const float wobBase = 0.005f * sr;
    const float fbGhost = 0.55f + 0.33f * set.ghostSize;
    const int comb1Len = (int) ((0.0371f + 0.03f * set.ghostSize) * sr);
    const int comb2Len = (int) ((0.0433f + 0.034f * set.ghostSize) * sr);
    const int preLen = (int) (0.025f * sr);
    const float bitsStep = 2.0f / std::pow (2.0f, 16.0f - 9.0f * set.lofi);
    const int crushHold = 1 + (int) (set.lofi * set.lofi * 5.0f);
    const float outG = juce::Decibels::decibelsToGain (set.outputDb);

    const float inc[7] = { twoPi * dblRate / sr, twoPi * set.chorusRate / sr, twoPi * set.flangeRate / sr,
                           twoPi * set.phaseRate / sr, twoPi * set.wobbleRate / sr, twoPi * 7.3f / sr,
                           twoPi * (0.12f + 0.5f * set.autopan) / sr };

    const float leadAtt = std::exp (-1.0f / (0.005f * sr)), leadRel = std::exp (-1.0f / (0.15f * sr));
    const float actAtt = std::exp (-1.0f / (0.02f * sr)), actRel = std::exp (-1.0f / (0.25f * sr));
    const float peakDecay = std::exp (-1.0f / (3.0f * sr));

    float* d[2] = { main.getWritePointer (0), chans > 1 ? main.getWritePointer (1) : nullptr };

    for (int i = 0; i < n; ++i)
    {
        // ---- lead vocal activity ----
        if (haveSc)
        {
            const float l = scL[i], r = scR != nullptr ? scR[i] : l;
            const float a = 0.5f * (std::fabs (l) + std::fabs (r));
            leadEnv = a + (a > leadEnv ? leadAtt : leadRel) * (leadEnv - a);
            leadPeak = juce::jmax (leadEnv, leadPeak * peakDecay);
            scSilentFor = a > 0.001f ? 0.0f : scSilentFor + 1.0f;
        }
        else scSilentFor += 1.0f;
        const bool present = scSilentFor < 2.0f * sr;
        const float target = present && leadPeak > 0.002f ? juce::jlimit (0.0f, 1.0f, (leadEnv / leadPeak - 0.12f) / 0.3f) : 0.0f;
        leadAct = target + (target > leadAct ? actAtt : actRel) * (leadAct - target);
        const float act = present ? leadAct : 0.0f;

        const float widthEff = juce::jlimit (0.0f, 1.0f, set.width + (present ? set.bloom * 0.35f * act : 0.0f));
        const float ghostEff = set.ghost * (present ? (1.0f + set.bloom * 1.2f * (1.0f - act)) * (1.0f - 0.3f * set.bloom * act) : 1.0f);
        const float duckG = present ? juce::Decibels::decibelsToGain (-set.duck * act) : 1.0f;

        float in[2] = { d[0][i], chans > 1 ? d[1][i] : d[0][i] };
        float dry[2], wet[2];

        for (int c = 0; c < 2; ++c)
        {
            auto& s = ch[(size_t) c];
            const float cp = (float) c;
            s.dryLine.push (in[c]);
            dry[c] = s.dryLine.tap (dryDelay - 1);

            // filter
            s.hp.tick (in[c]); s.lp.tick (s.hp.high);
            float x = s.lp.low;

            // dirt / lofi
            if (set.dirt > 0.001f)
                x = x * (1.0f - set.dirt) + std::tanh (dirtG * x) * dirtComp * set.dirt;
            if (set.lofi > 0.001f)
            {
                s.lofiLP.tick (x);
                float y = s.lofiLP.low;
                if (--s.crushCount <= 0) { s.crushCount = crushHold; s.crushHeld = std::round (y / bitsStep) * bitsStep; }
                x = x * (1.0f - set.lofi) + s.crushHeld * set.lofi;
            }

            // doubler: one detuned, delayed voice per side
            s.dblLine.push (x);
            if (set.dbl > 0.001f)
            {
                const float v = s.dblLine.read (dblBase + dblDepth * std::sin ((float) lfo[0] + cp * 3.14159f));
                x = x + set.dbl * v * 0.8f;
            }

            // chorus: three voices
            s.chorusLine.push (x);
            if (set.chorus > 0.001f)
            {
                float sum = 0;
                for (int k = 0; k < 3; ++k)
                    sum += s.chorusLine.read (chBase[k] + chDepth * (1.0f + std::sin ((float) lfo[1] * (1.0f + 0.13f * k) + k * 2.1f + cp * 1.57f)));
                x = x * (1.0f - 0.35f * set.chorus) + set.chorus * sum * 0.33f * 0.9f * 1.6f;
            }

            // flanger
            s.flangeLine.push (x + s.flangeFbState * set.flangeFb);
            if (set.flange > 0.001f)
            {
                float ph = (float) lfo[2] / twoPi + cp * 0.25f; ph -= std::floor (ph);
                const float tri = ph < 0.5f ? ph * 2.0f : 2.0f - ph * 2.0f;
                const float y = s.flangeLine.read ((0.0004f + 0.0045f * tri * tri) * sr);
                s.flangeFbState = y;
                x = (x + set.flange * y) / (1.0f + 0.5f * set.flange);
            }
            else s.flangeFbState = 0;

            // phaser: 6 allpass stages
            if (set.phase > 0.001f)
            {
                const float f = 200.0f * std::pow (20.0f, 0.5f + 0.5f * std::sin ((float) lfo[3] + cp * 1.57f));
                const float t = std::tan (3.14159f * juce::jmin (f, 0.45f * sr) / sr);
                const float a = (t - 1.0f) / (t + 1.0f);
                float u = x + s.phaseFbState * set.phaseFb;
                for (auto& st : s.apState) { const float y = a * u + st; st = u - a * y; u = y; }
                s.phaseFbState = u;
                x = x + set.phase * (u - x) * 0.5f;
            }

            // tape wobble (always through the delay so timing stays aligned with the dry path)
            s.wobbleLine.push (x);
            {
                const float wob = set.wobble * (0.0022f * sr * std::sin ((float) lfo[4] + cp * 0.3f) + 0.00012f * sr * std::sin ((float) lfo[5]));
                x = s.wobbleLine.read (wobBase + wob);
            }
            wet[c] = x;
        }

        // ---- width / autopan ----
        {
            const float m = 0.5f * (wet[0] + wet[1]);
            float sd = 0.5f * (wet[0] - wet[1]) * (widthEff * 2.0f);
            widthA.push (m); widthB.push (m);
            sd += widthHP.hp (widthA.tap ((int) (0.0071f * sr)) - widthB.tap ((int) (0.0113f * sr))) * widthEff * 0.45f;
            wet[0] = m + sd; wet[1] = m - sd;
            if (set.autopan > 0.001f)
            {
                const float pan = set.autopan * 0.75f * std::sin ((float) lfo[6]);
                wet[0] *= 1.0f - pan; wet[1] *= 1.0f + pan;
            }
        }

        // ---- ghost: diffuse tail with optional shimmer ----
        if (ghostEff > 0.001f)
        {
            const float send = 0.5f * (wet[0] + wet[1]);
            for (int c = 0; c < 2; ++c)
            {
                auto& s = ch[(size_t) c];
                s.pre.push (send);
                float y = s.pre.tap (preLen + c * 37);
                for (auto& a : s.diff) y = a.tick (y);
                if (set.shimmer > 0.001f) y = y * (1.0f - 0.5f * set.shimmer) + s.oct.tick (y) * set.shimmer;
                const float c1 = s.comb1.tap (comb1Len + c * 113);
                const float c2 = s.comb2.tap (comb2Len + c * 71);
                s.comb1.push (y + fbGhost * s.damp1.lp (c1));
                s.comb2.push (y + fbGhost * s.damp2.lp (c2));
                const float tail = s.ghostHP.hp ((c1 + c2) * 0.5f);
                wet[c] += tail * ghostEff * 0.9f;
            }
        }

        // auto level: keep the layer at the same loudness as it came in, whatever the preset
        {
            const float lvlCoef = std::exp (-1.0f / (1.2f * sr));
            const float ip = 0.5f * (dry[0] * dry[0] + dry[1] * dry[1]);
            const float wp = 0.5f * (wet[0] * wet[0] + wet[1] * wet[1]);
            inPowA = ip + lvlCoef * (inPowA - ip);
            wetPowA = wp + lvlCoef * (wetPowA - wp);
            if (inPowA > 1e-7f)
            {
                const float want = juce::jlimit (0.5f, 2.0f, std::sqrt (inPowA / (wetPowA + 1e-12f)));
                levelG = want + 0.9995f * (levelG - want);
            }
            wet[0] *= levelG; wet[1] *= levelG;
        }

        for (int c = 0; c < chans; ++c)
        {
            float y;
            if (chans == 1) y = dry[0] * (1.0f - set.mix) + 0.5f * (wet[0] + wet[1]) * set.mix;
            else            y = dry[c] * (1.0f - set.mix) + wet[c] * set.mix;
            y *= duckG * outG;
            d[c][i] = std::isfinite (y) ? y : 0.0f;
        }

        // wrap rarely (every ~1000 cycles) so the detuned chorus voices never jump
        for (int k = 0; k < 7; ++k) { lfo[k] += inc[k]; if (lfo[k] > 6283.185307179586) lfo[k] -= 6283.185307179586; }
    }

    leadActivity.store (leadAct);
    leadPresent.store (scSilentFor < 2.0f * sr);
    intensity.store (juce::jlimit (0.0f, 1.0f, set.mix * (0.35f * (set.chorus + set.flange + set.phase + set.wobble + set.dirt + set.lofi) + 0.8f * set.ghost + 0.3f * set.width)));
}

} // namespace poss
