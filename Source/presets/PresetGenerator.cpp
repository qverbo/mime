#include "PresetGenerator.h"
#include <functional>
#include <algorithm>
#include <vector>

namespace mime
{

namespace
{
    enum Family { Clean, Aggro, LoFi, Space };

    struct Archetype
    {
        juce::StringArray names;
        juce::String lane;
        juce::String shortDesc;
        juce::String whatItDoes;
        juce::String bestFor;
        Family family;
        bool spacey;
        std::function<float (const BeatProfile&)> fit;
        std::function<Settings (juce::Random&)> make;
    };

    float r (juce::Random& rng, float lo, float hi) { return lo + rng.nextFloat() * (hi - lo); }

    const std::vector<Archetype>& archetypes()
    {
        static const std::vector<Archetype> list = []
        {
            std::vector<Archetype> a;

            // 0
            a.push_back ({ { "Blown Speaker", "Redline", "Torn Cone" },
                "Ken Carson, Destroy Lonely",
                "Hard-clipped mids like a speaker pushed past its limit. Loud and in your face.",
                "Clips the middle of your voice the way a cheap speaker breaks up when you crank it. "
                "The lows stay clean so it never gets muddy, and a little sheen up top keeps it cutting.",
                "Hooks, high-energy verses, anything that needs to hit hard.",
                Aggro, false,
                [] (const BeatProfile& p) { return 0.5f + 0.3f * (1.0f - p.darkness) + 0.2f * p.subHeavy - 0.25f * p.ethereal; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Clip; s.drive = r (g, 0.55f, 0.8f); s.mix = r (g, 0.5f, 0.72f);
                    s.texture = (int) Texture::Sheen; s.air = r (g, 0.2f, 0.4f); s.body = r (g, 0.1f, 0.25f);
                    s.focus = r (g, 0.08f, 0.22f); s.fizz = 0.55f; s.width = r (g, 0.0f, 0.15f); s.tone = r (g, -0.05f, 0.15f);
                    s.xLow = r (g, 250, 380); s.xHigh = r (g, 4000, 5500);
                    return s;
                } });

            // 1
            a.push_back ({ { "Grave Dirt", "Basement Tape", "Six Feet" },
                "Sematary, Haunted Mound",
                "Crushed, grimy and buried. Lo-fi horror grit with the shine rolled off.",
                "Bit-crushes the body of your voice and folds the top end into a dirty rasp, then darkens it all, "
                "so it sounds like it was recorded on a busted machine in a basement. Band-limited so it stays out of the 808's way.",
                "Dark verses, horrorcore energy, doubles under a lead.",
                LoFi, false,
                [] (const BeatProfile& p) { return 0.4f + 0.35f * p.darkness + 0.15f * p.grit; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Crush; s.drive = r (g, 0.4f, 0.62f); s.mix = r (g, 0.6f, 0.85f);
                    s.texture = (int) Texture::Fold; s.air = r (g, 0.2f, 0.4f); s.body = r (g, 0.3f, 0.55f);
                    s.focus = r (g, 0.35f, 0.55f); s.tone = r (g, -0.45f, -0.2f); s.haze = r (g, 0.05f, 0.2f);
                    s.fizz = 0.5f; s.width = r (g, 0.05f, 0.2f);
                    return s;
                } });

            // 2
            a.push_back ({ { "Glass Halo", "Halo Glass", "Crystal Throat" },
                "2hollis",
                "Mostly clean voice with a glassy octave-up shimmer in the highs. Expensive-sounding.",
                "Keeps the core of your voice nearly clean with soft tape warmth, then adds an octave-up glass shimmer "
                "only to the top end. It reads as bright and polished. Distortion you feel more than hear.",
                "Melodic hooks, airy verses, when the words matter.",
                Clean, false,
                [] (const BeatProfile& p) { return 0.45f + 0.3f * p.darkness + 0.2f * p.ethereal; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Tape; s.drive = r (g, 0.18f, 0.35f); s.mix = r (g, 0.5f, 0.7f);
                    s.texture = (int) Texture::Octave; s.air = r (g, 0.4f, 0.7f); s.body = r (g, 0.05f, 0.15f);
                    s.tone = r (g, 0.1f, 0.3f); s.haze = r (g, 0.1f, 0.3f); s.width = r (g, 0.3f, 0.55f);
                    s.fizz = 0.4f; s.xHigh = r (g, 3500, 5000);
                    return s;
                } });

            // 3
            a.push_back ({ { "Seance", "Ouija", "Static Choir" },
                "elusin, Salem",
                "Crushed and washed into a ghostly haze. Classic witch house.",
                "Crushes your voice into a digital rasp, then smears that distortion into a wide, foggy tail "
                "so the vocal sounds like it's coming from another room. The dry voice stays underneath to hold the words.",
                "Witch house, intros, haunted hooks, layered chants.",
                Space, true,
                [] (const BeatProfile& p) { return 0.25f + 0.45f * p.ethereal + 0.15f * p.space + 0.2f * p.darkness; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Crush; s.drive = r (g, 0.3f, 0.5f); s.mix = r (g, 0.55f, 0.8f);
                    s.texture = (int) Texture::Fold; s.air = r (g, 0.1f, 0.3f); s.body = r (g, 0.1f, 0.3f);
                    s.focus = r (g, 0.25f, 0.45f); s.haze = r (g, 0.5f, 0.8f); s.width = r (g, 0.5f, 0.8f);
                    s.tone = r (g, -0.3f, -0.1f); s.fizz = 0.45f;
                    return s;
                } });

            // 4
            a.push_back ({ { "Velvet Tube", "Warm Valve", "Thick Glow" },
                "Any lane (the safe pick)",
                "Warm, thick tube saturation. Bigger and closer without sounding 'distorted'.",
                "Runs your voice through a warm tube-style saturation that thickens the mids and adds weight. "
                "It's subtle on purpose: the vocal just sounds more expensive and more forward.",
                "Main vocal on anything. A good starting point to tweak from.",
                Clean, false,
                [] (const BeatProfile& p) { return 0.55f + 0.2f * p.grit; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Tube; s.drive = r (g, 0.3f, 0.5f); s.mix = r (g, 0.35f, 0.55f);
                    s.texture = (int) Texture::Sheen; s.air = r (g, 0.1f, 0.25f); s.body = r (g, 0.2f, 0.4f);
                    s.fizz = 0.45f; s.tone = r (g, -0.05f, 0.1f);
                    return s;
                } });

            // 5
            a.push_back ({ { "Radio Hex", "Payphone", "Walkie" },
                "Sematary, Ken Carson ad-libs",
                "Squeezed through a tiny busted speaker. No lows, no highs, all grit.",
                "Band-limits the distortion down to a narrow, clipped midrange, like a voice coming through a broken radio. "
                "It jumps out of a dense beat because it lives in its own little slice of frequencies.",
                "Intros, ad-libs, a hook that needs to pop out, contrast sections.",
                Aggro, false,
                [] (const BeatProfile& p) { return 0.4f + 0.2f * p.grit + 0.1f * p.subHeavy; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Clip; s.drive = r (g, 0.4f, 0.62f); s.mix = r (g, 0.75f, 1.0f);
                    s.texture = (int) Texture::Clean; s.air = 0.0f; s.body = 0.0f;
                    s.focus = r (g, 0.75f, 0.95f); s.haze = r (g, 0.0f, 0.15f); s.fizz = 0.5f; s.tone = r (g, -0.1f, 0.1f);
                    return s;
                } });

            // 6
            a.push_back ({ { "Chrome Fuzz", "Pedalboard", "Buzzsaw" },
                "Ken Carson, Homixide Gang",
                "Thick, buzzing fuzz-pedal distortion with a metallic edge. Aggressive but smooth.",
                "Rectifier fuzz on the mids gives a buzzy, saturated wall of grit, like a guitar fuzz pedal on your voice. "
                "The fizz control keeps it from crackling, so it stays heavy without being harsh.",
                "Rage hooks, shouted sections, punchy beats with room in the mids.",
                Aggro, false,
                [] (const BeatProfile& p) { return 0.45f + 0.25f * (1.0f - p.space) + 0.15f * (1.0f - p.grit); },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Fuzz; s.drive = r (g, 0.55f, 0.85f); s.mix = r (g, 0.4f, 0.6f);
                    s.texture = (int) Texture::Sheen; s.air = r (g, 0.2f, 0.4f); s.body = r (g, 0.1f, 0.3f);
                    s.fizz = 0.6f; s.width = r (g, 0.1f, 0.3f); s.tone = r (g, -0.1f, 0.1f);
                    return s;
                } });

            // 7
            a.push_back ({ { "Shatter", "Mirror Fold", "Chrome Ring" },
                "2hollis, Nettspend",
                "Wave-folded into a metallic, almost robotic ring. Blended low for a futuristic edge.",
                "Wave-folding bends your voice back onto itself, which creates a metallic, ringing tone nothing else makes. "
                "It's blended in parallel so the words stay intact. It just adds a sci-fi edge.",
                "Futuristic hooks, weird ad-libs, standing out on a crowded beat.",
                Space, false,
                [] (const BeatProfile& p) { return 0.45f + 0.2f * p.darkness + 0.1f * p.space; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Fold; s.drive = r (g, 0.5f, 0.8f); s.mix = r (g, 0.25f, 0.45f);
                    s.texture = (int) Texture::Octave; s.air = r (g, 0.15f, 0.35f); s.body = r (g, 0.05f, 0.2f);
                    s.width = r (g, 0.25f, 0.45f); s.haze = r (g, 0.0f, 0.15f); s.fizz = 0.5f;
                    return s;
                } });

            // 8
            a.push_back ({ { "Siren Air", "Blade Air", "Hiss Halo" },
                "2hollis, Nettspend",
                "Body stays clean, only the air gets distorted. Cuts through like a blade.",
                "Leaves the body of your voice almost untouched and only saturates the very top: breaths, S's and air. "
                "The vocal slices through a dark beat without getting any heavier.",
                "Dark beats, quiet verses that still need to cut, airy melodies.",
                Clean, false,
                [] (const BeatProfile& p) { return 0.4f + 0.45f * p.darkness; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Tape; s.drive = r (g, 0.08f, 0.2f); s.mix = r (g, 0.6f, 0.8f);
                    s.texture = (int) Texture::Sheen; s.air = r (g, 0.6f, 0.9f); s.body = 0.05f;
                    s.xHigh = r (g, 4000, 6000); s.tone = r (g, 0.2f, 0.4f); s.fizz = 0.3f; s.width = r (g, 0.1f, 0.3f);
                    return s;
                } });

            // 9
            a.push_back ({ { "Cathedral Burn", "Candle Smoke", "Altar Haze" },
                "elusin, Salem, Crystal Castles",
                "Dry voice up front, a burning ghost-tail of distortion swirling behind it.",
                "Warm tube distortion that mostly lives in a big, diffuse tail around the voice. "
                "Your vocal stays clear in front while a glowing, distorted haze fills the space behind it.",
                "Witch house, slow ethereal beats, big atmospheric hooks.",
                Space, true,
                [] (const BeatProfile& p) { return 0.25f + 0.45f * p.ethereal + 0.1f * p.space + 0.15f * p.darkness; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Tube; s.drive = r (g, 0.5f, 0.72f); s.mix = r (g, 0.35f, 0.55f);
                    s.texture = (int) Texture::Sheen; s.air = r (g, 0.15f, 0.3f); s.body = r (g, 0.1f, 0.25f);
                    s.haze = r (g, 0.6f, 0.9f); s.width = r (g, 0.6f, 0.9f); s.focus = r (g, 0.1f, 0.25f);
                    s.tone = r (g, -0.2f, 0.0f); s.fizz = 0.45f;
                    return s;
                } });

            // 10
            a.push_back ({ { "Corrupted File", "Datamosh", "Glitch Saint" },
                "Sematary, elusin",
                "Low-blend digital crush that glitches around the edges. Clean voice, broken shadow.",
                "Heavily crushes a copy of your voice down to a low-res digital shadow and tucks it under the clean vocal, "
                "with crushed sparkle on top. It sounds like a corrupted file playing alongside you.",
                "Doubles, background layers, glitchy hooks, witch house.",
                LoFi, false,
                [] (const BeatProfile& p) { return 0.4f + 0.2f * p.ethereal + 0.2f * p.darkness; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Crush; s.drive = r (g, 0.62f, 0.85f); s.mix = r (g, 0.25f, 0.42f);
                    s.texture = (int) Texture::Crush; s.air = r (g, 0.35f, 0.6f); s.body = r (g, 0.0f, 0.15f);
                    s.width = r (g, 0.3f, 0.5f); s.haze = r (g, 0.1f, 0.25f); s.fizz = 0.5f;
                    return s;
                } });

            // 11
            a.push_back ({ { "Tape Ghost", "Cassette Demo", "Worn Reel" },
                "Haunted Mound, Sematary",
                "Warm, smeared tape distortion with the shine rolled off. Moody and lo-fi.",
                "Pushes your voice into warm tape saturation and rolls off the top, like an old cassette demo. "
                "Moody and lo-fi but still smooth, with no crackle and no harshness.",
                "Moody verses, lo-fi hooks, sitting the vocal inside a dark beat.",
                LoFi, false,
                [] (const BeatProfile& p) { return 0.4f + 0.25f * p.darkness + 0.15f * p.space; },
                [] (juce::Random& g)
                {
                    Settings s; s.style = (int) Style::Tape; s.drive = r (g, 0.4f, 0.62f); s.mix = r (g, 0.65f, 0.9f);
                    s.texture = (int) Texture::Clean; s.air = 0.0f; s.body = r (g, 0.2f, 0.4f);
                    s.tone = r (g, -0.45f, -0.25f); s.focus = r (g, 0.18f, 0.35f); s.haze = r (g, 0.15f, 0.3f);
                    s.width = r (g, 0.1f, 0.25f); s.fizz = 0.35f;
                    return s;
                } });

            // ---------------- expansion pack ----------------
            auto add = [&a] (juce::StringArray names, const char* lane, const char* shortD, const char* what, const char* best,
                             Family fam, bool spacey, std::function<float (const BeatProfile&)> fit, std::function<Settings (juce::Random&)> make)
            { a.push_back ({ names, lane, shortD, what, best, fam, spacey, fit, make }); };

            add ({ "Bleach", "Overexposed", "White Noise Halo" }, "glaive, 2hollis",
                 "Bright, glaring fuzz with an octave-up sparkle. Hyperpop at full brightness.",
                 "Fuzz on the mids plus an octave-up shimmer on the top, tilted bright. It sounds like the vocal is blown out by light instead of volume.",
                 "Hyperpop hooks, fast melodic runs, anything that should sound shiny and loud.",
                 Aggro, false, [] (const BeatProfile& p) { return 0.4f + 0.35f * (1.0f - p.darkness) + 0.15f * (1.0f - p.grit); },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Fuzz; s.drive = r (g, 0.4f, 0.6f); s.mix = r (g, 0.45f, 0.65f); s.texture = (int) Texture::Octave; s.air = r (g, 0.45f, 0.7f); s.tone = r (g, 0.2f, 0.4f); s.fizz = 0.55f; s.width = r (g, 0.15f, 0.35f); return s; });

            add ({ "Nail Gun", "Rivet", "Jackhammer" }, "Ken Carson, Che",
                 "Narrow, punchy hard clip that cuts straight through a loud beat.",
                 "A focused clipper on the middle of the voice with the lows and extreme highs trimmed off, so all the energy lands right where the ear is most sensitive.",
                 "Rage hooks, chants, shouted lines over a wall of 808s.",
                 Aggro, false, [] (const BeatProfile& p) { return 0.45f + 0.3f * p.grit + 0.15f * p.subHeavy - 0.2f * p.ethereal; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Clip; s.drive = r (g, 0.65f, 0.9f); s.mix = r (g, 0.55f, 0.8f); s.texture = (int) Texture::Clean; s.focus = r (g, 0.35f, 0.5f); s.body = 0.05f; s.fizz = 0.6f; return s; });

            add ({ "Funeral Lace", "Veil of Ash", "Lace Shroud" }, "Ecco2k, elusin",
                 "Barely-there tape warmth, glassy octave air and a wide, mournful haze.",
                 "The core of the voice stays almost clean while a glassy shimmer and a soft ghostly tail spread out to the sides. Delicate and sad.",
                 "Slow ethereal songs, quiet verses, intros and outros.",
                 Space, true, [] (const BeatProfile& p) { return 0.25f + 0.5f * p.ethereal + 0.2f * p.darkness; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Tape; s.drive = r (g, 0.1f, 0.25f); s.mix = r (g, 0.5f, 0.7f); s.texture = (int) Texture::Octave; s.air = r (g, 0.35f, 0.6f); s.haze = r (g, 0.45f, 0.7f); s.width = r (g, 0.6f, 0.85f); s.tone = r (g, 0.0f, 0.2f); s.fizz = 0.35f; return s; });

            add ({ "Rust Halo", "Oxide", "Corroded" }, "Sematary, Haunted Mound",
                 "Warm tube grit with crushed, rusty highs. Old, dark and dirty.",
                 "Tube saturation thickens the body while the top end gets bit-crushed into a rusty sizzle, then the whole thing is darkened like old metal.",
                 "Dark verses, horrorcore, layering under a cleaner lead.",
                 LoFi, false, [] (const BeatProfile& p) { return 0.4f + 0.35f * p.darkness + 0.1f * p.grit; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Tube; s.drive = r (g, 0.45f, 0.65f); s.mix = r (g, 0.55f, 0.8f); s.texture = (int) Texture::Crush; s.air = r (g, 0.3f, 0.5f); s.body = r (g, 0.25f, 0.45f); s.tone = r (g, -0.35f, -0.15f); s.focus = r (g, 0.1f, 0.25f); s.fizz = 0.5f; return s; });

            add ({ "Overdrive Angel", "Seraph Drive", "Burning Wings" }, "Bladee, Yung Lean",
                 "Pushed tube drive spread wide with a soft sheen. Heavenly but gritty.",
                 "A hard-driven tube tone blended in at half strength, with sheen on top and the distortion spread across the stereo field. The vocal sounds huge and slightly broken.",
                 "Melodic cloud-rap hooks, big choruses, emotional peaks.",
                 Clean, false, [] (const BeatProfile& p) { return 0.45f + 0.2f * p.ethereal + 0.15f * (1.0f - p.grit); },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Tube; s.drive = r (g, 0.6f, 0.8f); s.mix = r (g, 0.35f, 0.55f); s.texture = (int) Texture::Sheen; s.air = r (g, 0.3f, 0.5f); s.width = r (g, 0.5f, 0.8f); s.haze = r (g, 0.05f, 0.2f); s.fizz = 0.5f; return s; });

            add ({ "Pager", "Beeper", "Burner Phone" }, "Nettspend, 2hollis",
                 "Light digital crush squeezed into a bright, narrow band. Like a voice note from a cheap phone.",
                 "A gentle bit-crush on the mids, band-limited and tilted bright, so the vocal sounds like it's coming out of a tiny phone speaker without getting harsh.",
                 "Intros, ad-libs, contrast sections, half-time switch-ups.",
                 LoFi, false, [] (const BeatProfile& p) { return 0.45f + 0.15f * p.grit + 0.1f * (1.0f - p.darkness); },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Crush; s.drive = r (g, 0.25f, 0.4f); s.mix = r (g, 0.7f, 0.95f); s.texture = (int) Texture::Clean; s.focus = r (g, 0.55f, 0.75f); s.tone = r (g, 0.2f, 0.4f); s.fizz = 0.45f; return s; });

            add ({ "Molten", "Magma Throat", "Slag" }, "Destroy Lonely, Ken Carson",
                 "Thick, dark fuzz with folded edges and a heavy low end. Slow-burning menace.",
                 "Fuzz on the mids, a metallic fold on the top and extra low-end warmth, all darkened. It sounds heavy and molten rather than bright and loud.",
                 "Slow menacing verses, dark rage, low-register delivery.",
                 Aggro, false, [] (const BeatProfile& p) { return 0.4f + 0.3f * p.darkness + 0.15f * p.subHeavy; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Fuzz; s.drive = r (g, 0.6f, 0.8f); s.mix = r (g, 0.45f, 0.65f); s.texture = (int) Texture::Fold; s.air = r (g, 0.2f, 0.35f); s.body = r (g, 0.35f, 0.55f); s.tone = r (g, -0.4f, -0.2f); s.fizz = 0.55f; return s; });

            add ({ "Ghost Signal", "Dead Air", "Numbers Station" }, "Salem, elusin",
                 "Folded, band-limited voice drifting through haze. A transmission from somewhere else.",
                 "A wavefolded midrange squeezed into a radio-like band, smeared into a foggy tail and spread wide. Eerie and distant.",
                 "Witch house, intros, chants, background layers.",
                 Space, true, [] (const BeatProfile& p) { return 0.25f + 0.5f * p.ethereal + 0.15f * p.darkness; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Fold; s.drive = r (g, 0.35f, 0.55f); s.mix = r (g, 0.55f, 0.8f); s.texture = (int) Texture::Clean; s.focus = r (g, 0.5f, 0.7f); s.haze = r (g, 0.4f, 0.65f); s.width = r (g, 0.5f, 0.8f); s.tone = r (g, -0.15f, 0.05f); s.fizz = 0.45f; return s; });

            add ({ "Paper Cut", "Razor Thin", "Hairline" }, "Che, Nettspend",
                 "Barely clipped, with crisp air on top. Adds bite without sounding distorted.",
                 "A very light clip on the mids and a lot of sheen on the highs. The vocal gets a sharp, papery edge that cuts through but still sounds clean.",
                 "Main vocals that need to cut, fast flows, dense beats.",
                 Clean, false, [] (const BeatProfile& p) { return 0.45f + 0.2f * p.grit + 0.2f * p.darkness; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Clip; s.drive = r (g, 0.15f, 0.3f); s.mix = r (g, 0.55f, 0.8f); s.texture = (int) Texture::Sheen; s.air = r (g, 0.55f, 0.8f); s.tone = r (g, 0.1f, 0.25f); s.fizz = 0.35f; return s; });

            add ({ "Velour", "Crushed Velvet", "Plush" }, "Yung Lean, Bladee",
                 "Warm, heavy tape with a thick body and soft top. Smooth and close.",
                 "Pushes the voice into warm tape saturation with extra low-end body and a gently rolled-off top. Intimate and rich.",
                 "Moody verses, sung hooks, slower songs.",
                 Clean, false, [] (const BeatProfile& p) { return 0.45f + 0.2f * p.grit + 0.1f * (1.0f - p.darkness); },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Tape; s.drive = r (g, 0.35f, 0.55f); s.mix = r (g, 0.5f, 0.7f); s.texture = (int) Texture::Clean; s.body = r (g, 0.4f, 0.6f); s.tone = r (g, -0.3f, -0.1f); s.fizz = 0.35f; return s; });

            add ({ "Chainsaw", "Bandsaw", "Ripcord" }, "Ken Carson, Homixide Gang",
                 "Maximum fuzz blended in parallel. Brutal grit that never buries the words.",
                 "Fuzz cranked all the way up but mixed in low, so you get a ripping layer of grit under a still-readable vocal. The fizz control keeps it from shredding your ears.",
                 "The hardest rage hooks, screamed sections, drops.",
                 Aggro, false, [] (const BeatProfile& p) { return 0.4f + 0.2f * p.subHeavy + 0.15f * (1.0f - p.ethereal); },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Fuzz; s.drive = r (g, 0.85f, 1.0f); s.mix = r (g, 0.28f, 0.42f); s.texture = (int) Texture::Sheen; s.air = r (g, 0.2f, 0.35f); s.focus = r (g, 0.15f, 0.3f); s.fizz = 0.75f; return s; });

            add ({ "Moth Wing", "Dust Wing", "Lantern Moth" }, "2hollis, Ecco2k",
                 "Soft folding with octave flutter and a light haze. Fragile and fluttery.",
                 "A gentle wavefold gives the voice a papery, fluttering texture, with an octave-up glow on top and a light haze spreading it out.",
                 "Airy melodies, falsetto, delicate hooks.",
                 Space, false, [] (const BeatProfile& p) { return 0.35f + 0.3f * p.ethereal + 0.2f * p.darkness; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Fold; s.drive = r (g, 0.2f, 0.35f); s.mix = r (g, 0.4f, 0.6f); s.texture = (int) Texture::Octave; s.air = r (g, 0.3f, 0.5f); s.haze = r (g, 0.2f, 0.4f); s.width = r (g, 0.4f, 0.65f); s.fizz = 0.4f; return s; });

            add ({ "Cellar", "Root Cellar", "Crawlspace" }, "Haunted Mound, Sematary",
                 "Muffled tape with crushed sparkle and a short haze. Recorded in a basement.",
                 "Tape saturation, band-limited and darkened, with a little digital crush on top and a short tail, like a voice recorded through a floor.",
                 "Lo-fi verses, dark interludes, doubles.",
                 LoFi, false, [] (const BeatProfile& p) { return 0.4f + 0.35f * p.darkness + 0.1f * p.ethereal; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Tape; s.drive = r (g, 0.4f, 0.6f); s.mix = r (g, 0.7f, 0.9f); s.texture = (int) Texture::Crush; s.air = r (g, 0.15f, 0.3f); s.focus = r (g, 0.4f, 0.55f); s.tone = r (g, -0.4f, -0.2f); s.haze = r (g, 0.1f, 0.25f); s.fizz = 0.4f; return s; });

            add ({ "Neon Grave", "Strobe Crypt", "Laser Tomb" }, "Crystal Castles, Salem",
                 "Hard clip with a piercing octave edge, spread wide. Dark but electric.",
                 "Clipped mids, octave-up shimmer on top and a wide stereo spread. It's harsh and bright in the highs but sits on a dark, heavy core.",
                 "Electronic witch house, aggressive hooks, synth-heavy beats.",
                 Aggro, true, [] (const BeatProfile& p) { return 0.35f + 0.25f * p.ethereal + 0.2f * p.grit; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Clip; s.drive = r (g, 0.55f, 0.75f); s.mix = r (g, 0.45f, 0.65f); s.texture = (int) Texture::Octave; s.air = r (g, 0.4f, 0.6f); s.width = r (g, 0.5f, 0.75f); s.haze = r (g, 0.1f, 0.25f); s.fizz = 0.6f; return s; });

            add ({ "Hollow Tube", "Organ Pipe", "Drainpipe" }, "elusin, Salem",
                 "Tube grit squeezed into a hollow band with a short haze. Like singing into a pipe.",
                 "Tube saturation, band-limited so the voice sounds hollow and boxy, then smeared slightly so it rings like a pipe or tunnel.",
                 "Witch house, eerie ad-libs, background chants.",
                 LoFi, true, [] (const BeatProfile& p) { return 0.35f + 0.3f * p.ethereal + 0.15f * p.darkness; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Tube; s.drive = r (g, 0.4f, 0.6f); s.mix = r (g, 0.6f, 0.85f); s.texture = (int) Texture::Clean; s.focus = r (g, 0.3f, 0.45f); s.haze = r (g, 0.25f, 0.4f); s.width = r (g, 0.2f, 0.4f); s.fizz = 0.4f; return s; });

            add ({ "Static Bloom", "Snowfield", "Frost Static" }, "Bladee, Ecco2k",
                 "A low-blend digital crush blooming into a wide, soft haze. Fuzzy, frozen and dreamy.",
                 "A crushed copy of the voice tucked under the clean one, with polish on top and a wide haze. Like snow static on an old screen.",
                 "Dreamy hooks, cloud rap, background layers.",
                 Space, true, [] (const BeatProfile& p) { return 0.3f + 0.4f * p.ethereal + 0.15f * (1.0f - p.grit); },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Crush; s.drive = r (g, 0.35f, 0.55f); s.mix = r (g, 0.25f, 0.4f); s.texture = (int) Texture::Sheen; s.air = r (g, 0.3f, 0.5f); s.haze = r (g, 0.45f, 0.7f); s.width = r (g, 0.55f, 0.8f); s.fizz = 0.4f; return s; });

            add ({ "Teeth", "Jawbone", "Bite Mark" }, "Sematary, Ken Carson",
                 "Heavy folding plus crushed highs, blended low. Gnarly and snarling.",
                 "A hard wavefold on the mids with crushed top end, mixed in parallel. It adds a snarling, toothy edge under the vocal.",
                 "Aggressive ad-libs, horrorcore, doubles.",
                 Aggro, false, [] (const BeatProfile& p) { return 0.4f + 0.2f * p.grit + 0.2f * p.darkness; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Fold; s.drive = r (g, 0.7f, 0.9f); s.mix = r (g, 0.25f, 0.4f); s.texture = (int) Texture::Crush; s.air = r (g, 0.35f, 0.55f); s.fizz = 0.55f; s.width = r (g, 0.1f, 0.3f); return s; });

            add ({ "Satin Fuzz", "Soft Buzz", "Silk Static" }, "2hollis, Yung Lean",
                 "Gentle fuzz with sheen. A smooth, warm buzz instead of a harsh one.",
                 "Fuzz at low drive and medium blend with a polished top. The voice gets a soft, buzzing glow that sits nicely in a mix.",
                 "Sung hooks, melodic verses, anything that needs warmth with a little edge.",
                 Clean, false, [] (const BeatProfile& p) { return 0.45f + 0.15f * p.grit + 0.1f * p.darkness; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Fuzz; s.drive = r (g, 0.25f, 0.4f); s.mix = r (g, 0.35f, 0.5f); s.texture = (int) Texture::Sheen; s.air = r (g, 0.25f, 0.45f); s.fizz = 0.45f; s.width = r (g, 0.1f, 0.3f); return s; });

            add ({ "Bonefire", "Pyre", "Ember Bones" }, "Sematary, Haunted Mound",
                 "Heavy tube burn with folded crackle and smoke. Ritualistic and dark.",
                 "Hard-driven tube saturation with a crackling fold on top, a darker tone and a bit of smoky haze. Sounds like a voice through fire.",
                 "Dark hooks, chants, horrorcore, witch house.",
                 LoFi, true, [] (const BeatProfile& p) { return 0.35f + 0.3f * p.darkness + 0.2f * p.ethereal; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Tube; s.drive = r (g, 0.7f, 0.9f); s.mix = r (g, 0.45f, 0.65f); s.texture = (int) Texture::Fold; s.air = r (g, 0.25f, 0.45f); s.tone = r (g, -0.3f, -0.1f); s.haze = r (g, 0.2f, 0.35f); s.fizz = 0.55f; return s; });

            add ({ "Siren Shock", "Klaxon", "Alarm Bell" }, "glaive, 2hollis",
                 "Clipped, piercing and very bright. An alarm cutting through everything.",
                 "Clipped mids, octave-up glare on top and a bright tilt. It's intense, so use it for moments that need to jump out.",
                 "Hyperpop drops, one-line hooks, ad-libs that need to scream.",
                 Aggro, false, [] (const BeatProfile& p) { return 0.35f + 0.35f * (1.0f - p.darkness) + 0.1f * p.grit; },
                 [] (juce::Random& g) { Settings s; s.style = (int) Style::Clip; s.drive = r (g, 0.6f, 0.8f); s.mix = r (g, 0.5f, 0.7f); s.texture = (int) Texture::Octave; s.air = r (g, 0.5f, 0.75f); s.tone = r (g, 0.3f, 0.5f); s.focus = r (g, 0.1f, 0.2f); s.fizz = 0.65f; return s; });

            return a;
        }();
        return list;
    }

    // Shape a preset around the beat and explain why, in plain English.
    juce::StringArray adaptToBeat (Settings& s, const BeatProfile& p, bool spacey)
    {
        juce::StringArray why;
        if (! p.valid)
        {
            s.fizzFreq = 3200.0f;
            why.add ("Hit LISTEN (or drop your beat file on MIME) and this preset gets tailored to the beat.");
            return why;
        }

        s.fizzFreq = juce::jlimit (1500.0f, 6000.0f, p.busyFreq * 1.1f);
        s.fizz = juce::jmax (s.fizz, 0.3f + 0.35f * p.grit);
        why.add ("The fizz control is aimed at about " + juce::String (s.fizzFreq / 1000.0f, 1)
                 + " kHz, right where your beat is most crowded, so the grit doesn't fight it.");

        if (p.grit > 0.55f)
        {
            s.mix  *= 1.0f - 0.35f * (p.grit - 0.4f);
            s.drive *= 0.92f;
            why.add ("Your beat is already distorted, so the blend is pulled back to " + juce::String (juce::roundToInt (s.mix * 100.0f))
                     + "%. The clean voice keeps the words readable.");
        }
        else if (p.grit < 0.3f)
        {
            s.drive = juce::jmin (1.0f, s.drive * 1.1f);
            why.add ("The beat is pretty clean, so there's room to push the drive a little harder.");
        }

        if (p.darkness > 0.5f && s.texture != (int) Texture::Clean)
        {
            s.air = juce::jmin (1.0f, s.air + 0.15f * p.darkness);
            why.add ("The beat's top end is dark, so the texture is pushed up into the air where you have room to shine.");
        }
        else if (p.darkness < 0.3f)
        {
            s.air *= 0.7f;
            s.tone -= 0.15f;
            why.add ("The beat is already bright up top, so the highs are kept smooth to stay off the hi-hats.");
        }

        if (p.subHeavy > 0.6f)
        {
            s.lowCut = juce::jmax (s.lowCut, 110.0f + 40.0f * p.subHeavy);
            s.body *= 0.6f;
            why.add ("Heavy sub in the beat, so the vocal's low end is kept tight and clean.");
        }

        if (p.width > 0.65f)
        {
            if (spacey)
            {
                s.width = juce::jmin (1.0f, s.width + 0.1f);
                why.add ("The beat is wide and washy, so the haze spreads out to sit inside it.");
            }
            else
            {
                s.width *= 0.5f;
                why.add ("The beat is already wide, so the vocal distortion stays mostly in the center.");
            }
        }

        s.sanitise();
        return why;
    }

    juce::String composeLong (const Archetype& a, const juce::StringArray& why, const juce::String& extra = {})
    {
        juce::String t = a.whatItDoes;
        if (extra.isNotEmpty()) t << "\n\n" << extra;
        if (! why.isEmpty())
        {
            t << "\n\nWhy it fits this beat:";
            for (int i = 0; i < juce::jmin (3, why.size()); ++i)
                t << "\n- " << why[i];
        }
        t << "\n\nBest for: " << a.bestFor;
        return t;
    }
}

int PresetGenerator::numArchetypes() { return (int) archetypes().size(); }

juce::ValueTree Preset::toValueTree() const
{
    juce::ValueTree v ("Preset");
    v.setProperty ("name", name, nullptr);
    v.setProperty ("lane", lane, nullptr);
    v.setProperty ("short", shortDesc, nullptr);
    v.setProperty ("long", longDesc, nullptr);
    v.setProperty ("archetype", archetype, nullptr);
    v.setProperty ("locked", locked, nullptr);
    v.appendChild (settings.toValueTree(), nullptr);
    return v;
}

Preset Preset::fromValueTree (const juce::ValueTree& v)
{
    Preset p;
    p.name      = v.getProperty ("name").toString();
    p.lane      = v.getProperty ("lane").toString();
    p.shortDesc = v.getProperty ("short").toString();
    p.longDesc  = v.getProperty ("long").toString();
    p.archetype = v.getProperty ("archetype", -1);
    p.locked    = v.getProperty ("locked", false);
    p.settings  = Settings::fromValueTree (v.getChildWithName ("Settings"));
    return p;
}

PresetGenerator::PresetGenerator() : rng (juce::Time::currentTimeMillis()) {}

Preset PresetGenerator::build (int idx, const BeatProfile& beat, const juce::String& avoidName)
{
    const auto& a = archetypes()[(size_t) idx];
    Preset p;
    p.archetype = idx;

    juce::StringArray pool = a.names;
    for (auto& n : recentNames) pool.removeString (n);
    pool.removeString (avoidName);
    if (pool.isEmpty()) pool = a.names;
    p.name = pool[rng.nextInt (pool.size())];

    p.lane = a.lane;
    p.shortDesc = a.shortDesc;
    p.settings = a.make (rng);
    auto why = adaptToBeat (p.settings, beat, a.spacey);
    p.longDesc = composeLong (a, why);
    return p;
}

void PresetGenerator::generate (const BeatProfile& beat)
{
    const auto& list = archetypes();
    const int n = (int) list.size();

    std::vector<float> w ((size_t) n);
    for (int i = 0; i < n; ++i)
    {
        float f = juce::jlimit (0.05f, 2.0f, list[(size_t) i].fit (beat));
        f = f * f;
        if (std::find (recent.begin(), recent.end(), i) != recent.end()) f *= 0.3f;
        w[(size_t) i] = f;
    }

    // keep locked presets out of the draw
    for (auto& s : slots)
        if (s.locked && s.archetype >= 0) w[(size_t) s.archetype] = 0.0f;

    std::vector<int> picks;
    int freeSlots = 0;
    for (auto& s : slots) if (! s.locked) ++freeSlots;

    for (int k = 0; k < freeSlots; ++k)
    {
        float total = 0; for (auto x : w) total += x;
        int choice = 0;
        if (total <= 0.0f) choice = rng.nextInt (n);
        else
        {
            float t = rng.nextFloat() * total;
            for (int i = 0; i < n; ++i) { t -= w[(size_t) i]; if (t <= 0) { choice = i; break; } choice = i; }
        }
        picks.push_back (choice);
        w[(size_t) choice] = 0.0f;
        // diversity: make the same family less likely next draw
        for (int i = 0; i < n; ++i)
            if (list[(size_t) i].family == list[(size_t) choice].family) w[(size_t) i] *= 0.5f;
    }

    std::vector<juce::String> newNames;
    size_t pi = 0;
    for (auto& s : slots)
    {
        if (s.locked) continue;
        const auto oldName = s.name;
        s = build (picks[pi++], beat, oldName);
        newNames.push_back (s.name);
    }

    recent.clear();
    for (auto& s : slots) if (s.archetype >= 0 && ! s.locked) recent.push_back (s.archetype);
    for (auto& nme : newNames) { recentNames.push_back (nme); if (recentNames.size() > 12) recentNames.pop_front(); }
}

void PresetGenerator::moreLikeThis (int index, const BeatProfile& beat)
{
    if (index < 0 || index >= numSlots || slots[(size_t) index].isEmpty()) return;
    const Preset base = slots[(size_t) index];
    const auto& a = archetypes()[(size_t) base.archetype];

    struct Twist { const char* tag; const char* sentence; std::function<void (Settings&, juce::Random&)> apply; };
    std::vector<Twist> twists = {
        { "Dirtier",  "This version pushes the drive and blend harder.",
          [] (Settings& s, juce::Random& g) { s.drive += r (g, 0.12f, 0.22f); s.mix += r (g, 0.08f, 0.15f); } },
        { "Cleaner",  "This version backs off the drive and blend so more of your natural voice comes through.",
          [] (Settings& s, juce::Random& g) { s.drive -= r (g, 0.1f, 0.2f); s.mix -= r (g, 0.1f, 0.18f); } },
        { "Wider",    "This version spreads the distortion wider and adds more haze around it.",
          [] (Settings& s, juce::Random& g) { s.width += r (g, 0.2f, 0.35f); s.haze += r (g, 0.15f, 0.3f); } },
        { "Darker",   "This version rolls off the top for a moodier, more buried sound.",
          [] (Settings& s, juce::Random& g) { s.tone -= r (g, 0.25f, 0.4f); s.air -= r (g, 0.1f, 0.2f); s.focus += r (g, 0.0f, 0.15f); } },
        { "Brighter", "This version opens up the top with more air and sparkle.",
          [] (Settings& s, juce::Random& g) { s.tone += r (g, 0.2f, 0.35f); s.air += r (g, 0.15f, 0.25f); } },
        { "Weirder",  "This version swaps the top-end texture for something stranger.",
          [] (Settings& s, juce::Random& g)
          {
              int t = s.texture;
              while (t == s.texture) t = 1 + g.nextInt ((int) Texture::NumTextures - 1);
              s.texture = t; s.air = juce::jmax (s.air, r (g, 0.3f, 0.55f));
          } },
    };

    // shuffle
    for (int i = (int) twists.size() - 1; i > 0; --i) std::swap (twists[(size_t) i], twists[(size_t) rng.nextInt (i + 1)]);

    size_t t = 0;
    for (int i = 0; i < numSlots; ++i)
    {
        auto& s = slots[(size_t) i];
        if (i == index || s.locked) continue;
        const auto& tw = twists[t++ % twists.size()];

        Preset v = base;
        v.locked = false;
        tw.apply (v.settings, rng);
        // small random jitter so repeats are never identical
        v.settings.drive += r (rng, -0.03f, 0.03f);
        v.settings.mix   += r (rng, -0.03f, 0.03f);
        v.settings.sanitise();
        v.name = base.name.upToFirstOccurrenceOf (" / ", false, false) + " / " + tw.tag;
        v.shortDesc = base.shortDesc;

        Settings tmp = v.settings;
        auto why = adaptToBeat (tmp, beat, a.spacey); // only for the explanation; keep twisted settings
        juce::ignoreUnused (tmp);
        v.longDesc = composeLong (a, why, tw.sentence);
        s = v;
    }
}

juce::ValueTree PresetGenerator::toValueTree() const
{
    juce::ValueTree v ("Presets");
    for (auto& s : slots) v.appendChild (s.toValueTree(), nullptr);
    return v;
}

void PresetGenerator::fromValueTree (const juce::ValueTree& v)
{
    if (! v.isValid()) return;
    for (int i = 0; i < juce::jmin (numSlots, v.getNumChildren()); ++i)
        slots[(size_t) i] = Preset::fromValueTree (v.getChild (i));
}

} // namespace mime
