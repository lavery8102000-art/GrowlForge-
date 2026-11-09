// Headless test: render a raw saw bass through every preset and sanity-check the output.
#include "../Source/PluginProcessor.h"
#include <cmath>
#include <cstdio>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;

    const double sr = 48000.0;
    const int blockSize = 512;
    const int totalBlocks = 200; // ~2.1 s
    int failures = 0;

    GrowlForgeProcessor proc;
    proc.setPlayConfigDetails (2, 2, sr, blockSize);

    for (int preset = 0; preset < proc.getNumPrograms(); ++preset)
    {
        proc.setCurrentProgram (preset);
        proc.prepareToPlay (sr, blockSize);

        juce::AudioBuffer<float> buf (2, blockSize);
        juce::MidiBuffer midi;
        double phase = 0.0;
        const double freq = 55.0; // A1 saw

        double peak = 0.0, sumSq = 0.0;
        long count = 0;
        bool bad = false;

        for (int b = 0; b < totalBlocks; ++b)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const float s = (float) (0.5 * (2.0 * phase - 1.0));
                buf.setSample (0, i, s);
                buf.setSample (1, i, s);
                phase += freq / sr;
                if (phase >= 1.0) phase -= 1.0;
            }

            proc.processBlock (buf, midi);

            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < blockSize; ++i)
                {
                    const float v = buf.getSample (ch, i);
                    if (! std::isfinite (v)) bad = true;
                    peak = juce::jmax (peak, (double) std::abs (v));
                    if (b > 20) { sumSq += (double) v * v; ++count; }
                }
        }

        const double rmsDb = 20.0 * std::log10 (std::sqrt (sumSq / (double) count) + 1e-12);
        const double peakDb = 20.0 * std::log10 (peak + 1e-12);
        const bool pass = ! bad && peakDb < 0.5 && rmsDb > -40.0;
        std::printf ("%-18s  peak %6.2f dBFS   rms %6.2f dBFS   %s\n",
                     proc.getProgramName (preset).toRawUTF8(), peakDb, rmsDb, pass ? "OK" : "FAIL");
        if (! pass) ++failures;
    }

    std::printf (failures == 0 ? "\nALL PRESETS PASSED\n" : "\n%d FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
