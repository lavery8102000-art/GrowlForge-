#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <memory>
#include <vector>

// GrowlForge: a bass-design effect. Put it after any synth (e.g. a plain saw)
// and it turns the raw tone into a vowel-morphing, comb-resonant, multiband-distorted growl.
class GrowlForgeProcessor : public juce::AudioProcessor
{
public:
    GrowlForgeProcessor();
    ~GrowlForgeProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "GrowlForge"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentPreset; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    static const juce::StringArray& lfoRateNames();
    static const juce::StringArray& lfoShapeNames();

private:
    // ---- DSP helpers ----
    struct Comb
    {
        void prepare (double sr);
        void clear();
        float process (int ch, float x, float delaySamples, float feedback);
        std::array<std::vector<float>, 2> buf;
        std::array<int, 2> w { 0, 0 };
        int size = 0;
    };

    static constexpr int kSubBlock = 32;
    static constexpr int kNumFormants = 3;

    float readParam (const juce::String& id) const;
    float lfoValue (double phase, bool wrapped);

    double sr = 44100.0;
    std::array<juce::dsp::StateVariableTPTFilter<float>, kNumFormants> formant;
    Comb comb;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::dsp::LinkwitzRileyFilter<float> split1, split2;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::DryWetMixer<float> mixer { 1024 };

    juce::SmoothedValue<float> driveSmooth, outSmooth;

    double lfoPhase = 0.0, lastPhase = 0.0;
    float heldRandom = 0.0f;
    juce::Random rng;
    int currentPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrowlForgeProcessor)
};
