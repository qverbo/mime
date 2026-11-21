#include "HexPattern.h"

namespace hex
{

namespace
{
    // relative weight of each effect per style (index = Fx)
    const float styleWeights[(int) Style::NumStyles][(int) Fx::NumFx] = {
        //            thru stut roll rev  tape  -8   +8   gate scat
        /* GLITCH */ { 0,  4,   3,   1,   0.5f, 0.5f, 3,  2,   2 },
        /* HAUNT  */ { 0,  1,   0.3f,4,   3,    3,   0.3f,1,  2 },
        /* CHOP   */ { 0,  3,   1,   0.5f,1,    1.5f,0.5f,4,  1 },
        /* FILL   */ { 0,  3,   3,   2,   2,    1,   1.5f,1,  1 },
        /* CHAOS  */ { 0,  1,   1,   1,   1,    1,   1,   1,  1 },
    };

    int pick (const float* w, juce::Random& rng)
    {
        float total = 0;
        for (int i = 1; i < (int) Fx::NumFx; ++i) total += w[i];
        float t = rng.nextFloat() * total;
        for (int i = 1; i < (int) Fx::NumFx; ++i) { t -= w[i]; if (t <= 0) return i; }
        return (int) Fx::Stutter;
    }
}

juce::String PatternGenerator::makeName (juce::Random& rng)
{
    static const char* words[] = { "OMEN", "VEIL", "ASH", "WRAITH", "COVEN", "RUNE", "CRYPT", "MOTH", "THORN", "SHADE",
                                   "RELIC", "HOLLOW", "SIGIL", "EMBER", "NOCTURNE", "VESSEL", "GRAVE", "HALO", "SPECTER", "MIRE" };
    static const char* numerals[] = { "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI", "XII", "XIII" };
    return juce::String (words[rng.nextInt (20)]) + " " + numerals[rng.nextInt (13)];
}

Pattern PatternGenerator::make (Style style, float density, juce::Random& rng)
{
    Pattern p;
    const float* w = styleWeights[(int) style];
    density = juce::jlimit (0.05f, 1.0f, density);

    int i = 0;
    while (i < numSteps)
    {
        // where in the bar are we? keep downbeats cleaner so the groove survives
        float prob = density * 0.55f;
        if (i % 4 == 0) prob *= 0.45f;
        if (i % 2 == 1) prob *= 1.15f;
        if (style == Style::Fill) prob = i >= 12 ? 0.55f + 0.45f * density : density * 0.08f;
        if (style == Style::Chaos) prob = density * 0.8f;
        if (i >= 14) prob *= 1.3f;  // ends of bars like a fill

        if (rng.nextFloat() < prob)
        {
            const int fx = pick (w, rng);
            // some effects sound best held across more than one step
            int run = 1;
            if (fx == (int) Fx::TapeStop || fx == (int) Fx::OctDown || fx == (int) Fx::Reverse)
                run = 1 + rng.nextInt (style == Style::Haunt ? 3 : 2);
            else if (fx == (int) Fx::Roll && rng.nextFloat() < 0.4f)
                run = 2;
            else if (fx == (int) Fx::Gate && style == Style::Chop)
                run = 1;

            for (int r = 0; r < run && i < numSteps; ++r, ++i)
                p.steps[(size_t) i] = fx;
        }
        else
        {
            ++i;
        }
    }

    // chop style: lock in a rhythmic gate figure
    if (style == Style::Chop && rng.nextBool())
        for (int s = 2; s < numSteps; s += 4)
            if (p.steps[(size_t) s] == 0) p.steps[(size_t) s] = (int) Fx::Gate;

    // never generate an empty pattern
    if (p.isEmpty())
        p.steps[(size_t) (12 + rng.nextInt (4))] = pick (w, rng);

    p.name = makeName (rng);
    return p;
}

} // namespace hex
