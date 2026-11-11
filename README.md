# AeroCrush

A glitch / lo-fi / abstract hip-hop effect for FL Studio on Mac (VST3 + AU, Apple Silicon and Intel).
Sequenced glitches in the spirit of the SP-404MKII, tone-wrecking in the spirit of RC-20, wrapped in a glossy Frutiger Aero UI.

## Build on your Mac (about 10 minutes, once)

1. Install **Xcode** from the App Store, open it once to accept the licence.
2. Install **CMake**: `brew install cmake` (or download from cmake.org).
3. In Terminal, from this folder:

```bash
cmake -B build -G Xcode
cmake --build build --config Release
```

The first run downloads JUCE (needs internet). When it finishes, the plugin is already installed to:

- `~/Library/Audio/Plug-Ins/VST3/AeroCrush.vst3`
- `~/Library/Audio/Plug-Ins/Components/AeroCrush.component` (AU)

A standalone app is also built in `build/AeroCrush_artefacts/Release/Standalone/`.

**No Xcode?** Push this folder to a GitHub repo. The included workflow builds it on GitHub's Macs; download `AeroCrush-mac` from the Actions tab and copy the two bundles into the folders above. Downloaded builds are quarantined by macOS, so clear that once:

```bash
xattr -cr ~/Library/Audio/Plug-Ins/VST3/AeroCrush.vst3 ~/Library/Audio/Plug-Ins/Components/AeroCrush.component
```

## Load it in FL Studio

Options → Manage plugins → **Find installed plugins**, then star AeroCrush. Put it on a mixer insert (drums, a sample chop, or the master bus).

## What's inside

Signal flow: **Glitch Sequencer → Isolator → Filter → Drive → Digital → Wobble → Magnetic → Noise → Space → Output**.
Every module has its own power pill; everything is automatable in FL.

| Module | Controls | What it does |
|---|---|---|
| Glitch Sequencer | 16 steps, Rate (1/4–1/32), Prob, Chaos, Slices, Mix, Clear / Randomize / Fill | Tempo-synced step glitches locked to FL's playhead. Free-runs at project tempo when stopped. |
| Digital | Bits, Rate | Bit-crush and sample-rate reduction (SP-style crunch) |
| Drive | Drive, Tone | Asymmetric tube-ish saturation with a tone low-pass |
| Wobble | Wow, Flutter | Tape pitch drift and fast flutter |
| Magnetic | Dropout, Depth | Random tape dropouts and level flux |
| Noise | Hiss, Crackle | Tape hiss and vinyl pops |
| Filter | Low Cut, High Cut, Reso | Resonant band-limiting |
| Isolator | Low, Mid, High | 3-band kill EQ (-40 dB = full kill) |
| Space | Size, Mix | Reverb |
| Output | Width, Dry/Wet, Gain | Stereo width, global mix, level |

**Step effects** (click a step to cycle, right-click or ctrl-click to go back):

- **Stutter** – repeats the slice just before the step; *Slices* sets how many repeats
- **Reverse** – plays the previous step backwards
- **Tape Stop** – slows to a halt over the step
- **Half Speed** – octave-down slowed playback
- **Gate** – rhythmic chop; *Slices* sets the chop rate
- **Scatter** – jumps to a random earlier slice
- **Mute** – drops out

*Prob* = chance a programmed step actually fires. *Chaos* = chance any step (even an empty one) turns into a random effect. Both make each bar different.

## Factory presets (16)

Init · SP Lo-Fi Dust · Stutter Bounce · Tape Stop Endings · Reverse Dreams · Abstract Chaos · Cassette Warble · Broken Sampler · Underwater Isolator · Half-Time Haze · Gate Chop · Aero Bubbles · Radio Ghost · Drunk Machine · Floppy Disk Kit · Skyline Shimmer

Pick them from the dropdown at the top right, or step with `<` `>`. Save your own tweaks with FL's own preset menu (the plugin state, including the step pattern, is stored with your project).

## Notes

- Turning **Wobble** on adds about 12 ms of delay to the signal (tape-style); leave it on for the whole track if timing matters.
- Factory presets live in `Source/Presets.h`; add your own there using the step letters `. S R T H G X M`.
- DSP self-test: `ctest --test-dir build -C Release`.
