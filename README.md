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
