#include "PossPresets.h"
#include <functional>
#include <algorithm>

namespace poss
{

namespace
{
    float r (juce::Random& g, float lo, float hi) { return lo + g.nextFloat() * (hi - lo); }

    struct Archetype
    {
        juce::StringArray names;
        juce::String lane, desc;
        std::array<float, (int) Role::NumRoles> roleFit;   // DOUBLE HARMONY ADLIB WHISPER LOW
        bool spacey;
        std::function<void (PossSettings&, juce::Random&)> make;
    };

    const std::vector<Archetype>& archetypes()
    {
        static const std::vector<Archetype> a = []
        {
            std::vector<Archetype> v;
            v.push_back ({ { "Velvet Double", "Second Skin", "Twin Throat" }, "any lane",
                "A tight, natural double. Two slightly detuned copies thicken the layer without it sounding like an effect.",
                { 1.0f, 0.5f, 0.3f, 0.3f, 0.6f }, false,
                [] (PossSettings& s, juce::Random& g) { s.dbl = r (g, 0.45f, 0.7f); s.detune = r (g, 8, 16); s.dblMs = r (g, 14, 24); s.chorus = r (g, 0, 0.12f); s.width = r (g, 0.35f, 0.55f); } });

            v.push_back ({ { "Choir of Ghosts", "Hollow Choir", "Pale Chorus" }, "bladee / ecco2k",
                "Lush three-voice ensemble with a soft shimmering tail. One voice becomes a distant choir.",
                { 0.3f, 1.0f, 0.4f, 0.8f, 0.4f }, true,
                [] (PossSettings& s, juce::Random& g) { s.chorus = r (g, 0.55f, 0.8f); s.chorusRate = r (g, 0.3f, 0.7f); s.chorusDepth = r (g, 0.5f, 0.8f); s.ghost = r (g, 0.3f, 0.5f); s.ghostSize = r (g, 0.5f, 0.8f); s.shimmer = r (g, 0.2f, 0.45f); s.width = r (g, 0.7f, 0.95f); } });

            v.push_back ({ { "Jet Halo", "Contrail", "Halo Sweep" }, "2hollis",
                "A slow, airy flanger sweep that makes the layer shimmer and move like a jet passing overhead.",
                { 0.3f, 0.8f, 0.9f, 0.6f, 0.3f }, false,
                [] (PossSettings& s, juce::Random& g) { s.flange = r (g, 0.5f, 0.8f); s.flangeRate = r (g, 0.06f, 0.18f); s.flangeFb = r (g, 0.4f, 0.7f); s.dbl = r (g, 0.15f, 0.3f); s.width = r (g, 0.5f, 0.8f); } });

            v.push_back ({ { "Seance Phase", "Ouija Drift", "Candle Phase" }, "salem / elusin",
                "Slow phaser and a lo-fi haze. The layer breathes in and out like it's coming through a wall.",
                { 0.4f, 0.7f, 0.7f, 0.8f, 0.6f }, true,
                [] (PossSettings& s, juce::Random& g) { s.phase = r (g, 0.6f, 0.9f); s.phaseRate = r (g, 0.08f, 0.3f); s.phaseFb = r (g, 0.4f, 0.7f); s.lofi = r (g, 0.15f, 0.35f); s.ghost = r (g, 0.2f, 0.4f); s.width = r (g, 0.5f, 0.8f); } });

            v.push_back ({ { "Warped Cassette", "Worn Reel", "Melted Tape" }, "haunted mound / sematary",
                "Wobbly, warped tape with grit and a rolled-off top. Like a layer pulled off an old demo tape.",
                { 0.4f, 0.4f, 0.8f, 0.3f, 1.0f }, false,
                [] (PossSettings& s, juce::Random& g) { s.wobble = r (g, 0.4f, 0.75f); s.wobbleRate = r (g, 0.5f, 1.4f); s.dirt = r (g, 0.2f, 0.45f); s.lofi = r (g, 0.3f, 0.55f); s.width = r (g, 0.25f, 0.5f); } });

            v.push_back ({ { "Silver Veil", "Frost Veil", "Mirror Silk" }, "ecco2k / bladee",
                "Bright, glassy and wide. Soft chorus plus an octave-up shimmer that floats above the lead.",
                { 0.3f, 0.9f, 0.4f, 1.0f, 0.1f }, true,
                [] (PossSettings& s, juce::Random& g) { s.chorus = r (g, 0.3f, 0.5f); s.chorusRate = r (g, 0.5f, 1.0f); s.shimmer = r (g, 0.4f, 0.7f); s.ghost = r (g, 0.3f, 0.5f); s.ghostSize = r (g, 0.3f, 0.6f); s.width = r (g, 0.8f, 1.0f); } });

            v.push_back ({ { "Iron Flange", "Chrome Throat", "Steel Wing" }, "ken carson / destroy lonely",
                "Metallic, jet-engine flanger with negative feedback. Aggressive and robotic, great on ad-libs.",
                { 0.3f, 0.3f, 1.0f, 0.3f, 0.5f }, false,
                [] (PossSettings& s, juce::Random& g) { s.flange = r (g, 0.7f, 1.0f); s.flangeRate = r (g, 0.2f, 0.6f); s.flangeFb = r (g, -0.8f, -0.55f); s.dirt = r (g, 0.15f, 0.35f); s.width = r (g, 0.4f, 0.7f); } });

            v.push_back ({ { "Drowned Chorus", "Undertow", "Deep Water" }, "yung lean / elusin",
                "Deep, slow chorus with a gentle tape warble and a darker top. Sounds underwater and dreamy.",
                { 0.4f, 0.8f, 0.4f, 0.4f, 0.9f }, false,
                [] (PossSettings& s, juce::Random& g) { s.chorus = r (g, 0.6f, 0.9f); s.chorusRate = r (g, 0.12f, 0.35f); s.chorusDepth = r (g, 0.7f, 1.0f); s.wobble = r (g, 0.15f, 0.35f); s.highCut = r (g, 6000, 9000); s.width = r (g, 0.5f, 0.75f); } });

            v.push_back ({ { "Wraith Whisper", "Breath Ghost", "Cold Breath" }, "salem / crystal castles",
                "Paper-thin and airy with a huge ghostly tail drifting side to side. Made for whispers.",
                { 0.1f, 0.4f, 0.5f, 1.0f, 0.1f }, true,
                [] (PossSettings& s, juce::Random& g) { s.ghost = r (g, 0.55f, 0.85f); s.ghostSize = r (g, 0.6f, 1.0f); s.shimmer = r (g, 0.1f, 0.35f); s.autopan = r (g, 0.3f, 0.6f); s.width = r (g, 0.85f, 1.0f); s.chorus = r (g, 0.1f, 0.3f); } });

            v.push_back ({ { "Blown Stack", "Torn Stack", "Redline Double" }, "ken carson / sematary",
                "Distorted double with a phaser edge. A gritty wall behind the lead for rage hooks.",
                { 0.8f, 0.3f, 0.8f, 0.1f, 0.6f }, false,
                [] (PossSettings& s, juce::Random& g) { s.dirt = r (g, 0.45f, 0.75f); s.dbl = r (g, 0.4f, 0.6f); s.phase = r (g, 0.2f, 0.45f); s.phaseRate = r (g, 0.4f, 1.0f); s.width = r (g, 0.45f, 0.7f); } });

            v.push_back ({ { "Mirror Room", "Glass Hall", "Twin Mirror" }, "2hollis / nettspend",
                "Wide double with a faint flange shimmer. The layer sits on both sides of the lead like mirrors.",
                { 0.9f, 0.7f, 0.5f, 0.4f, 0.3f }, false,
                [] (PossSettings& s, juce::Random& g) { s.dbl = r (g, 0.5f, 0.75f); s.detune = r (g, 12, 22); s.dblMs = r (g, 20, 32); s.flange = r (g, 0.15f, 0.3f); s.flangeRate = r (g, 0.1f, 0.25f); s.width = r (g, 0.8f, 1.0f); } });

            v.push_back ({ { "Possessed", "Exorcism", "Legion" }, "elusin / sematary",
                "Everything at once: chorus, flange, phase, wobble and a dark tail. Unstable and haunted.",
                { 0.2f, 0.4f, 1.0f, 0.6f, 0.5f }, true,
                [] (PossSettings& s, juce::Random& g) { s.chorus = r (g, 0.3f, 0.5f); s.flange = r (g, 0.3f, 0.5f); s.phase = r (g, 0.3f, 0.5f); s.wobble = r (g, 0.2f, 0.4f); s.ghost = r (g, 0.3f, 0.5f); s.dirt = r (g, 0.1f, 0.3f); s.width = r (g, 0.6f, 0.9f); } });

            v.push_back ({ { "Cathedral Stack", "Nave", "Bell Tower" }, "crystal castles / salem",
                "Ensemble plus a big cathedral-sized tail. Turns a stack of harmonies into a wall of sound.",
                { 0.2f, 1.0f, 0.3f, 0.5f, 0.7f }, true,
                [] (PossSettings& s, juce::Random& g) { s.chorus = r (g, 0.4f, 0.6f); s.ghost = r (g, 0.55f, 0.8f); s.ghostSize = r (g, 0.8f, 1.0f); s.shimmer = r (g, 0.0f, 0.2f); s.width = r (g, 0.7f, 0.95f); } });

            v.push_back ({ { "Static Spirit", "Dead Channel", "Signal Ghost" }, "sematary / bladee",
                "Crushed, lo-fi and panning back and forth with a fast phaser. A broken radio ghost.",
                { 0.1f, 0.3f, 1.0f, 0.8f, 0.4f }, false,
                [] (PossSettings& s, juce::Random& g) { s.lofi = r (g, 0.45f, 0.75f); s.phase = r (g, 0.4f, 0.7f); s.phaseRate = r (g, 0.8f, 2.0f); s.autopan = r (g, 0.4f, 0.8f); s.dirt = r (g, 0.1f, 0.3f); s.width = r (g, 0.5f, 0.8f); } });
            return v;
        }();
        return a;
    }

    // role-specific placement: where the layer sits around the lead
    void applyRole (PossSettings& s, int role, juce::Random& g)
    {
        s.role = role;
        switch ((Role) role)
        {
            case Role::Double:  s.lowCut = r (g, 100, 140); s.highCut = juce::jmin (s.highCut, r (g, 13000, 16000)); s.width = juce::jmin (s.width, 0.6f); s.duck = r (g, 1.5f, 3.0f); s.bloom = r (g, 0.1f, 0.3f); s.dbl = juce::jmax (s.dbl, 0.3f); break;
            case Role::Harmony: s.lowCut = r (g, 160, 220); s.highCut = juce::jmin (s.highCut, r (g, 10000, 13000)); s.width = juce::jmax (s.width, 0.6f); s.duck = r (g, 3.0f, 5.0f); s.bloom = r (g, 0.3f, 0.6f); break;
            case Role::Adlib:   s.lowCut = r (g, 180, 260); s.highCut = juce::jmin (s.highCut, r (g, 8000, 11000)); s.autopan = juce::jmax (s.autopan, r (g, 0.15f, 0.4f)); s.duck = r (g, 2.0f, 4.0f); s.bloom = r (g, 0.3f, 0.6f); break;
            case Role::Whisper: s.lowCut = r (g, 300, 450); s.highCut = juce::jmin (s.highCut, r (g, 15000, 18000)); s.width = juce::jmax (s.width, 0.85f); s.ghost = juce::jmax (s.ghost, 0.35f); s.duck = r (g, 4.0f, 6.0f); s.bloom = r (g, 0.5f, 0.8f); break;
            case Role::Low:     s.lowCut = r (g, 50, 80); s.highCut = juce::jmin (s.highCut, r (g, 4500, 7000)); s.width = juce::jmin (s.width, 0.45f); s.dirt = juce::jmax (s.dirt, 0.15f); s.duck = r (g, 2.0f, 4.0f); s.bloom = r (g, 0.1f, 0.3f); break;
            default: break;
        }
    }

    void adaptToBeat (PossSettings& s, const mime::BeatProfile& p, bool spacey)
    {
        if (! p.valid) return;
        if (p.grit > 0.55f) s.dirt *= 0.6f;                          // beat already dirty
        if (p.darkness > 0.5f) s.shimmer = juce::jmin (1.0f, s.shimmer + 0.15f * p.darkness);
        if (p.ethereal > 0.55f && spacey) s.ghost = juce::jmin (1.0f, s.ghost + 0.1f);
        if (p.width > 0.7f && ! spacey) s.width *= 0.85f;           // keep some focus on wide beats
        if (p.subHeavy > 0.6f) s.lowCut = juce::jmax (s.lowCut, 90.0f);
    }
}

juce::ValueTree Preset::toValueTree() const
{
    juce::ValueTree v ("PossPreset");
    v.setProperty ("name", name, nullptr); v.setProperty ("lane", lane, nullptr); v.setProperty ("desc", desc, nullptr);
    v.setProperty ("archetype", archetype, nullptr); v.setProperty ("locked", locked, nullptr);
    v.appendChild (settings.toValueTree(), nullptr);
    return v;
}

Preset Preset::fromValueTree (const juce::ValueTree& v)
{
    Preset p;
    p.name = v.getProperty ("name").toString(); p.lane = v.getProperty ("lane").toString(); p.desc = v.getProperty ("desc").toString();
    p.archetype = v.getProperty ("archetype", -1); p.locked = v.getProperty ("locked", false);
    p.settings = PossSettings::fromValueTree (v.getChildWithName ("Poss"));
    return p;
}

PresetBank::PresetBank() : rng (juce::Time::currentTimeMillis()) {}

Preset PresetBank::build (int idx, int role, const mime::BeatProfile& beat, const juce::String& avoid)
{
    const auto& a = archetypes()[(size_t) idx];
    Preset p;
    p.archetype = idx;
    juce::StringArray pool = a.names;
    for (auto& n : recentNames) pool.removeString (n);
    pool.removeString (avoid);
    if (pool.isEmpty()) pool = a.names;
    p.name = pool[rng.nextInt (pool.size())];
    p.lane = a.lane;
    p.desc = a.desc;
    PossSettings s;
    s.dbl = 0; s.width = 0.5f;
    a.make (s, rng);
    applyRole (s, role, rng);
    adaptToBeat (s, beat, a.spacey);
    s.sanitise();
    p.settings = s;
    return p;
}

void PresetBank::generate (int role, const mime::BeatProfile& beat)
{
    const auto& list = archetypes();
    const int n = (int) list.size();
    std::vector<float> w ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        float f = list[(size_t) i].roleFit[(size_t) juce::jlimit (0, (int) Role::NumRoles - 1, role)];
        if (beat.valid && list[(size_t) i].spacey) f *= 0.8f + 0.5f * beat.ethereal;
        f = std::pow (f, 2.5f);
        if (std::find (recent.begin(), recent.end(), i) != recent.end()) f *= 0.3f;
        w[(size_t) i] = f;
    }
    for (auto& s : slots) if (s.locked && s.archetype >= 0) w[(size_t) s.archetype] = 0;

    std::vector<int> picks;
    int freeSlots = 0; for (auto& s : slots) if (! s.locked) ++freeSlots;
    for (int k = 0; k < freeSlots; ++k)
    {
        float total = 0; for (auto x : w) total += x;
        int choice = 0;
        if (total <= 0) choice = rng.nextInt (n);
        else { float t = rng.nextFloat() * total; for (int i = 0; i < n; ++i) { t -= w[(size_t) i]; choice = i; if (t <= 0) break; } }
        picks.push_back (choice);
        w[(size_t) choice] = 0;
    }

    size_t pi = 0;
    recent.clear();
    for (auto& s : slots)
    {
        if (s.locked) continue;
        s = build (picks[pi++], role, beat, s.name);
        recent.push_back (s.archetype);
        recentNames.push_back (s.name);
        if (recentNames.size() > 12) recentNames.pop_front();
    }
}

void PresetBank::moreLikeThis (int index, int role, const mime::BeatProfile& beat)
{
    if (index < 0 || index >= numSlots || slots[(size_t) index].isEmpty()) return;
    juce::ignoreUnused (beat);
    const Preset base = slots[(size_t) index];

    struct Twist { const char* tag; std::function<void (PossSettings&, juce::Random&)> f; };
    std::vector<Twist> twists = {
        { "Wider",    [] (PossSettings& s, juce::Random& g) { s.width = juce::jmin (1.0f, s.width + r (g, 0.2f, 0.35f)); s.autopan += r (g, 0.0f, 0.2f); } },
        { "Hazier",   [] (PossSettings& s, juce::Random& g) { s.ghost += r (g, 0.2f, 0.35f); s.ghostSize += r (g, 0.1f, 0.3f); } },
        { "Weirder",  [] (PossSettings& s, juce::Random& g) { s.flange += r (g, 0.2f, 0.4f); s.phase += r (g, 0.2f, 0.4f); s.wobble += r (g, 0.1f, 0.3f); } },
        { "Cleaner",  [] (PossSettings& s, juce::Random& g) { for (auto* x : { &s.chorus, &s.flange, &s.phase, &s.wobble, &s.ghost, &s.dirt, &s.lofi }) *x *= r (g, 0.4f, 0.65f); } },
        { "Dirtier",  [] (PossSettings& s, juce::Random& g) { s.dirt += r (g, 0.2f, 0.35f); s.lofi += r (g, 0.15f, 0.3f); } },
        { "Brighter", [] (PossSettings& s, juce::Random& g) { s.shimmer += r (g, 0.2f, 0.4f); s.highCut = juce::jmin (18000.0f, s.highCut * 1.3f); } },
    };
    for (int i = (int) twists.size() - 1; i > 0; --i) std::swap (twists[(size_t) i], twists[(size_t) rng.nextInt (i + 1)]);

    size_t t = 0;
    for (int i = 0; i < numSlots; ++i)
    {
        auto& s = slots[(size_t) i];
        if (i == index || s.locked) continue;
        const auto& tw = twists[t++ % twists.size()];
        Preset v = base;
        v.locked = false;
        tw.f (v.settings, rng);
        v.settings.role = role;
        v.settings.sanitise();
        v.name = base.name.upToFirstOccurrenceOf (" / ", false, false) + " / " + tw.tag;
        s = v;
    }
}

juce::ValueTree PresetBank::toValueTree() const
{
    juce::ValueTree v ("PossBank");
    for (auto& s : slots) v.appendChild (s.toValueTree(), nullptr);
    return v;
}

void PresetBank::fromValueTree (const juce::ValueTree& v)
{
    if (! v.isValid()) return;
    for (int i = 0; i < juce::jmin (numSlots, v.getNumChildren()); ++i)
        slots[(size_t) i] = Preset::fromValueTree (v.getChild (i));
}

} // namespace poss
