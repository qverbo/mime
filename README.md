# MIME

**Reads the beat. Makes the voice shine.**

MIME is a vocal distortion/saturation plugin (VST3, Windows). It listens to your beat, figures out what's already going on in it (how distorted, how dark, how wide, where it's crowded), then builds a set of distortion presets made for *that* beat. Each preset comes with a plain-English explanation and an artist lane it's in the spirit of.

## Install (FL Studio, Windows)

1. Download `MIME-Windows.zip` from the latest **Release** (or from the **Actions** tab → newest run → *Artifacts*).
2. Unzip it and copy the `MIME.vst3` folder to `C:\Program Files\Common Files\VST3\`.
3. In FL Studio, open **Options → Manage plugins → Find installed plugins**, then search for **MIME**.
4. Put MIME on your **vocal** mixer insert.

There's also a standalone `MIME Standalone.exe` if you want to open it outside FL.

## Let MIME hear the beat

**Easiest:** drag your beat's audio file (WAV/MP3/FLAC/AIFF) onto the MIME window, or click **LOAD BEAT FILE**.

**Live (sidechain):**
1. Select your **beat's** mixer track, then right-click the arrow at the bottom of MIME's **vocal** track → **Sidechain to this track**.
2. Open MIME's plugin wrapper (the gear icon in the top-left of the plugin window) → **Processing** tab, and set the sidechain input to the beat's track.
3. Hit **LISTEN** and play the beat. A fuller section works best. MIME listens for up to 20 seconds; click again to finish early (it needs at least 4s).

## Using the presets

- **Click a card** to load it. The details panel explains what it does, why it fits your beat, and what it's best for.
- **🔒 Lock** the cards you like, then hit **REGENERATE** to roll new ones into the other slots. It avoids repeating what you just saw.
- **MORE LIKE THIS** fills the unlocked slots with variations of the selected preset: dirtier, cleaner, wider, darker, brighter, weirder.
- Everything is saved with your FL project, including the beat read and the presets.

## The knobs

| Knob | What it does |
|---|---|
| **Style** | Main flavor: Tube (warm), Tape (soft), Clip (blown speaker), Fold (metallic ring), Crush (digital/lo-fi), Fuzz (buzzy pedal) |
| **Texture** | What happens to the top end: Clean, Sheen (polish), Crush (digital sparkle), Octave (glassy shimmer), Fold (metallic) |
| **Drive** | How hard the main distortion is pushed |
| **Mix** | Distorted vs clean voice. Lower keeps the words clear |
| **Body** | Warmth in the low end (never harsh) |
| **Air** | Amount of top-end texture |
| **Focus** | Narrows the distortion; all the way up gives a busted-radio sound |
| **Haze** | Smears the distortion into a ghostly tail |
| **Width** | Spreads the distortion out (the clean voice stays centered and it's mono-safe) |
| **Tone** | Darker ↔ brighter distortion |
| **Fizz** | Turns down harsh fizz only when it builds up; aimed where your beat is crowded |
| **Low Cut** | Cleans rumble before anything gets distorted |
| **Auto Gain** | Keeps output loudness matched to input so louder doesn't fool you |

Under the hood: 4x oversampling, a phase-matched 3-band split (low/mid/high), and input level tracking, so presets behave the same whether your vocal was recorded hot or quiet.

## Building from source

```
cmake -B build
cmake --build build --config Release --target MIME_VST3
```

JUCE 8 is downloaded automatically. Pushing to `main` builds Windows on GitHub Actions; pushing a `v*` tag also publishes a Release.

---

# FILIPINA RAGE

**Crunches your vocal every time the 808 (or the kick) hits.** Dedicated to my beautiful girlfriend.

Pick what triggers it at the top of the scope: **808** for trap and rage (long booming bass hits), or **KICK** for dance, house and four-on-the-floor (short punchy hits; works even with a bassline under the kick). Clicking either one loads matching timing.

Put it on your vocal, feed the beat (or just the 808 track) into its sidechain, and the vocal gets distorted only when the 808 lands. Between hits the vocal stays clean, or as dirty as you set **Idle**.

## Setup (FL Studio)
1. Download `FILIPINA-RAGE.vst3` from the latest Release and drag it into `C:\Program Files\Common Files\VST3\`, then rescan.
2. Put FILIPINA RAGE on your **vocal** insert.
3. Select the **808** (or beat) mixer track, then right-click the arrow at the bottom of the vocal track → **Sidechain to this track**.
4. In FILIPINA RAGE's wrapper (gear icon) → **Processing**, set the sidechain input to that track.
5. Pick **808** or **KICK**, play the beat and hit **LEARN**. It finds your 808 or kick and tunes the detector in about 6 seconds.

**Tip for KICK mode:** if your kick is on its own mixer track, sidechain just that track. Detection is then basically perfect. On a full beat, keep Sensitivity around 40%; higher can start catching bass notes.

## Controls
| | |
|---|---|
| **Crunch type** | Tube, Tape, Clip, Fuzz, Fold, Crush, Octave, Megaphone, Destroy |
| **Amount** | How hard the crunch hits |
| **Hit Mix** | How much crunched voice you hear during a hit |
| **Slam** | Extra volume push on every hit |
| **Idle** | Crunch between hits (0 = clean) |
| **Attack / Hold / Release** | How fast it grabs, how long it stays, how long it fades |
| **Mode** | *Hit*: every 808 fires a crunch. *Follow*: the crunch rides the 808's volume (long 808s keep crunching) |
| **Trigger On** | 808 (long bass hits) or KICK (short punchy hits) |
| **Sensitivity** | Higher = quieter hits trigger too |
| **808 / Kick Freq** | Where the detector listens (LEARN sets it) |
| **Hear detector** | Hear what the detector hears, for tuning. Turn it off after! |
| **TAP** | Fire a hit by hand to audition the crunch |

---

# HEX GATE

Tempo-synced vocal glitcher. Sixteen steps around the sigil; each one can stutter, roll, reverse, tape-stop, drop or jump an octave, gate, or scatter. Locks to FL's tempo and only glitches while the song plays.

- **Paint** steps with the brush (click, drag, right-click to erase).
- **Pattern bank**: pick a style (Glitch, Haunt, Chop, Fill, Chaos), hit **REGEN** for six new patterns, click one to load it, lock the ones you love.
- **Chaos** adds random glitches every bar. **Grid** sets step size (1/8, 1/16, 1/32). **Repeats** sets stutter speed. **Smooth** sets how hard the edits cut. **Swing** grooves it.

Tip: put HEX GATE on a vocal ad-lib bus or the last word of a phrase, and use the **Fill** style for end-of-bar edits.
