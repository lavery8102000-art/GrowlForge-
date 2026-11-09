# GrowlForge

A bass-design effect plugin (VST3) for dubstep, riddim and tearout. Put it after any synth
(a plain saw or square is perfect) and it shapes the tone into a vowel-morphing, comb-resonant,
multiband-distorted growl.

## Signal chain

1. **Drive** - input gain into a tanh saturator. **Growl** is a macro that pushes drive,
   formant mix and distortion together.
2. **Formant** - three parallel band-pass filters morphing across A / E / I / O / U.
3. **Comb** - tuned feedback comb filter (set Pitch in semitones) for the metallic edge.
4. **Distortion** - 3 bands (split at 180 Hz and 1.8 kHz), each with its own amount,
   moving from soft clip to wavefold. Runs at 2x oversampling.
5. **LFO** - tempo-synced (1/1 to 1/32), shapes: sine, triangle, saw, square, random.
   Depth knobs route it to Vowel, Comb pitch and Drive (bipolar).
6. **Output** - level, limiter ceiling, dry/wet mix. Reports its latency to the host.

Six factory presets are included (change them from the dropdown, top right).

## Getting the Windows build (for Mixcraft)

1. Create a new GitHub repository and upload everything in this folder (not needed: `build/`, `JUCE/`).
2. Open the **Actions** tab. The "Build GrowlForge (Windows VST3)" workflow runs on every push
   (or press *Run workflow*). It takes roughly 5-10 minutes.
3. Download the **GrowlForge-VST3-Windows** artifact and unzip it.
4. Copy the `GrowlForge.vst3` folder to `C:\Program Files\Common Files\VST3\`.
5. In Mixcraft: *Preferences > VST Plug-ins > Rescan* (or add that folder), then insert
   GrowlForge as an effect on the track holding your synth.

Or build locally on Windows with Visual Studio 2022 and CMake:

    cmake -S . -B build -A x64 -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --target GrowlForge_VST3

## Tests

`OfflineTest` renders a saw bass through every preset and checks for NaNs, clipping and silence.
