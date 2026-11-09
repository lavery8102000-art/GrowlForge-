#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

using APVTS = juce::AudioProcessorValueTreeState;

// ------------------------------------------------------------------------------------
// Parameters
// ------------------------------------------------------------------------------------
const juce::StringArray& GrowlForgeProcessor::lfoRateNames()
{
    static const juce::StringArray n { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" };
    return n;
}

const juce::StringArray& GrowlForgeProcessor::lfoShapeNames()
{
    static const juce::StringArray n { "Sine", "Triangle", "Saw", "Square", "Random" };
    return n;
}

APVTS::ParameterLayout GrowlForgeProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    using R = juce::NormalisableRange<float>;
    using ID = juce::ParameterID;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    auto pct = [] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; };
    auto db  = [] (float v, int) { return juce::String (v, 1) + " dB"; };

    p.push_back (std::make_unique<F> (ID { "drive", 1 }, "Drive", R (0.0f, 36.0f, 0.01f), 6.0f,
                                      juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    p.push_back (std::make_unique<F> (ID { "growl", 1 }, "Growl", R (0.0f, 1.0f, 0.001f), 0.0f,
                                      juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<F> (ID { "vowel", 1 }, "Vowel", R (0.0f, 1.0f, 0.001f), 0.25f));
    p.push_back (std::make_unique<F> (ID { "formRes", 1 }, "Formant Reso", R (1.0f, 16.0f, 0.01f, 0.6f), 5.0f));
    p.push_back (std::make_unique<F> (ID { "formMix", 1 }, "Formant Mix", R (0.0f, 1.0f, 0.001f), 0.7f,
                                      juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<F> (ID { "combPitch", 1 }, "Comb Pitch", R (24.0f, 96.0f, 0.01f), 48.0f));
    p.push_back (std::make_unique<F> (ID { "combFb", 1 }, "Comb Feedback", R (0.0f, 0.95f, 0.001f), 0.55f));
    p.push_back (std::make_unique<F> (ID { "combMix", 1 }, "Comb Mix", R (0.0f, 1.0f, 0.001f), 0.35f,
                                      juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    p.push_back (std::make_unique<F> (ID { "distLow", 1 },  "Dist Low",  R (0.0f, 1.0f, 0.001f), 0.2f));
    p.push_back (std::make_unique<F> (ID { "distMid", 1 },  "Dist Mid",  R (0.0f, 1.0f, 0.001f), 0.6f));
    p.push_back (std::make_unique<F> (ID { "distHigh", 1 }, "Dist High", R (0.0f, 1.0f, 0.001f), 0.4f));

    p.push_back (std::make_unique<C> (ID { "lfoRate", 1 }, "LFO Rate", lfoRateNames(), 2));
    p.push_back (std::make_unique<C> (ID { "lfoShape", 1 }, "LFO Shape", lfoShapeNames(), 0));
    p.push_back (std::make_unique<F> (ID { "lfoVowel", 1 }, "LFO > Vowel", R (-1.0f, 1.0f, 0.001f), 0.0f));
    p.push_back (std::make_unique<F> (ID { "lfoComb", 1 },  "LFO > Comb",  R (-1.0f, 1.0f, 0.001f), 0.0f));
    p.push_back (std::make_unique<F> (ID { "lfoDrive", 1 }, "LFO > Drive", R (-1.0f, 1.0f, 0.001f), 0.0f));

    p.push_back (std::make_unique<F> (ID { "outGain", 1 }, "Output", R (-24.0f, 12.0f, 0.01f), -3.0f,
                                      juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    p.push_back (std::make_unique<F> (ID { "ceiling", 1 }, "Ceiling", R (-12.0f, 0.0f, 0.01f), -0.5f,
                                      juce::AudioParameterFloatAttributes().withStringFromValueFunction (db)));
    p.push_back (std::make_unique<F> (ID { "mix", 1 }, "Mix", R (0.0f, 1.0f, 0.001f), 1.0f,
                                      juce::AudioParameterFloatAttributes().withStringFromValueFunction (pct)));

    return { p.begin(), p.end() };
}

// ------------------------------------------------------------------------------------
// Presets
// ------------------------------------------------------------------------------------
namespace
{
    struct PresetValue { const char* id; float value; };
    struct Preset { const char* name; std::vector<PresetValue> values; };

    const std::vector<Preset>& presets()
    {
        static const std::vector<Preset> list
        {
            { "Init", {} },
            { "Tearout Growl",
              { { "drive", 14 }, { "growl", 0.6f }, { "vowel", 0.35f }, { "formRes", 8 }, { "formMix", 0.9f },
                { "combPitch", 41 }, { "combFb", 0.7f }, { "combMix", 0.45f },
                { "distLow", 0.25f }, { "distMid", 0.9f }, { "distHigh", 0.7f },
                { "lfoRate", 3 }, { "lfoShape", 1 }, { "lfoVowel", 0.6f }, { "lfoComb", 0.15f }, { "lfoDrive", 0.2f } } },
            { "Riddim Wobble",
              { { "drive", 10 }, { "vowel", 0.15f }, { "formRes", 6 }, { "formMix", 0.85f },
                { "combPitch", 48 }, { "combFb", 0.5f }, { "combMix", 0.25f },
                { "distLow", 0.15f }, { "distMid", 0.7f }, { "distHigh", 0.35f },
                { "lfoRate", 2 }, { "lfoShape", 3 }, { "lfoVowel", 0.8f }, { "lfoDrive", 0.1f } } },
            { "Yoi Screech",
              { { "drive", 18 }, { "vowel", 0.6f }, { "formRes", 11 }, { "formMix", 1.0f },
                { "combPitch", 60 }, { "combFb", 0.8f }, { "combMix", 0.5f },
                { "distLow", 0.1f }, { "distMid", 0.8f }, { "distHigh", 0.95f },
                { "lfoRate", 4 }, { "lfoShape", 0 }, { "lfoVowel", 0.5f }, { "lfoComb", 0.35f } } },
            { "Deep Vowel Sub",
              { { "drive", 5 }, { "vowel", 0.8f }, { "formRes", 4 }, { "formMix", 0.6f },
                { "combPitch", 36 }, { "combFb", 0.3f }, { "combMix", 0.15f },
                { "distLow", 0.1f }, { "distMid", 0.3f }, { "distHigh", 0.1f },
                { "lfoRate", 1 }, { "lfoShape", 0 }, { "lfoVowel", 0.35f } } },
            { "Metal Comb Gnash",
              { { "drive", 12 }, { "vowel", 0.45f }, { "formRes", 7 }, { "formMix", 0.5f },
                { "combPitch", 55 }, { "combFb", 0.9f }, { "combMix", 0.8f },
                { "distLow", 0.3f }, { "distMid", 0.85f }, { "distHigh", 0.85f },
                { "lfoRate", 5 }, { "lfoShape", 4 }, { "lfoComb", 0.5f }, { "lfoDrive", 0.25f } } },
        };
        return list;
    }
}

int GrowlForgeProcessor::getNumPrograms() { return (int) presets().size(); }

const juce::String GrowlForgeProcessor::getProgramName (int index)
{
    if (juce::isPositiveAndBelow (index, getNumPrograms()))
        return presets()[(size_t) index].name;
    return {};
}

void GrowlForgeProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms()))
        return;

    currentPreset = index;

    // Reset everything to defaults first, then apply the preset's overrides.
    for (auto* param : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (param))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    for (auto& pv : presets()[(size_t) index].values)
        if (auto* rp = apvts.getParameter (pv.id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (pv.value));
}

// ------------------------------------------------------------------------------------
// Comb
// ------------------------------------------------------------------------------------
void GrowlForgeProcessor::Comb::prepare (double sampleRate)
{
    size = (int) std::ceil (sampleRate / 20.0) + 8; // lowest comb frequency ~20 Hz
    for (auto& b : buf)
        b.assign ((size_t) size, 0.0f);
    w = { 0, 0 };
}

void GrowlForgeProcessor::Comb::clear()
{
    for (auto& b : buf)
        std::fill (b.begin(), b.end(), 0.0f);
    w = { 0, 0 };
}

float GrowlForgeProcessor::Comb::process (int ch, float x, float delaySamples, float feedback)
{
    auto& b = buf[(size_t) ch];
    int& wi = w[(size_t) ch];

    delaySamples = juce::jlimit (2.0f, (float) (size - 4), delaySamples);
    float rp = (float) wi - delaySamples;
    if (rp < 0.0f) rp += (float) size;

    const int i0 = (int) rp;
    const int i1 = (i0 + 1) % size;
    const float frac = rp - (float) i0;
    const float delayed = b[(size_t) i0] + frac * (b[(size_t) i1] - b[(size_t) i0]);

    // Soft-clipped feedback keeps the loop stable at high settings.
    const float y = x + feedback * std::tanh (delayed);
    b[(size_t) wi] = juce::jlimit (-8.0f, 8.0f, y);
    wi = (wi + 1) % size;
    return y;
}

// ------------------------------------------------------------------------------------
// Processor
// ------------------------------------------------------------------------------------
GrowlForgeProcessor::GrowlForgeProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "GrowlForgeState", createLayout())
{
}

bool GrowlForgeProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

float GrowlForgeProcessor::readParam (const juce::String& id) const
{
    return apvts.getRawParameterValue (id)->load();
}

void GrowlForgeProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 2 };

    for (auto& f : formant)
    {
        f.prepare (spec);
        f.setType (juce::dsp::StateVariableTPTFilterType::bandpass);
        f.reset();
    }

    comb.prepare (sampleRate);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampler->initProcessing ((size_t) samplesPerBlock);

    juce::dsp::ProcessSpec osSpec { sampleRate * 2.0, (juce::uint32) samplesPerBlock * 2, 2 };
    split1.prepare (osSpec);
    split2.prepare (osSpec);
    split1.setCutoffFrequency (180.0f);
    split2.setCutoffFrequency (1800.0f);
    split1.reset();
    split2.reset();

    limiter.prepare (spec);
    limiter.setRelease (80.0f);
    limiter.reset();

    const auto latency = (int) std::ceil (oversampler->getLatencyInSamples());
    setLatencySamples (latency);
    mixer.prepare (spec);
    mixer.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    mixer.setWetLatency ((float) latency);
    mixer.reset();

    driveSmooth.reset (sampleRate, 0.02);
    outSmooth.reset (sampleRate, 0.02);
    driveSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (readParam ("drive")));
    outSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (readParam ("outGain")));

    lfoPhase = lastPhase = 0.0;
}

float GrowlForgeProcessor::lfoValue (double phase, bool wrapped)
{
    switch ((int) readParam ("lfoShape"))
    {
        case 0:  return std::sin ((float) (phase * juce::MathConstants<double>::twoPi));
        case 1:  return (float) (phase < 0.5 ? 4.0 * phase - 1.0 : 3.0 - 4.0 * phase);
        case 2:  return (float) (1.0 - 2.0 * phase);
        case 3:  return phase < 0.5 ? 1.0f : -1.0f;
        default:
            if (wrapped) heldRandom = rng.nextFloat() * 2.0f - 1.0f;
            return heldRandom;
    }
}

namespace
{
    // Formant table: F1, F2, F3 for A E I O U (bass-friendly, slightly low voice)
    constexpr float kVowels[5][3] = {
        { 800.0f, 1150.0f, 2800.0f },
        { 400.0f, 1600.0f, 2700.0f },
        { 350.0f, 1700.0f, 2700.0f },
        { 450.0f,  800.0f, 2830.0f },
        { 325.0f,  700.0f, 2530.0f },
    };
    constexpr float kFormantGain[3] = { 1.0f, 0.7f, 0.45f };

    inline float shape (float x, float amount)
    {
        if (amount <= 0.0001f)
            return x;

        const float g = 1.0f + amount * 24.0f;
        const float driven = g * x;
        const float clip = std::tanh (driven);
        const float fold = std::sin (driven * 0.9f);
        const float foldAmt = juce::jmax (0.0f, amount - 0.5f) * 2.0f;
        const float shaped = clip * (1.0f - foldAmt) + fold * foldAmt;
        const float wet = juce::jmin (1.0f, amount * 4.0f);
        return x + wet * (shaped - x);
    }
}

void GrowlForgeProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const int numSamples = buffer.getNumSamples();
    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    // ---- read parameters ----
    const float drive    = readParam ("drive");
    const float growl    = readParam ("growl");
    const float vowel    = readParam ("vowel");
    const float formRes  = readParam ("formRes");
    const float formMix  = readParam ("formMix");
    const float combPitch = readParam ("combPitch");
    const float combFb   = readParam ("combFb");
    const float combMix  = readParam ("combMix");
    const float distLow  = readParam ("distLow");
    const float distMid  = readParam ("distMid");
    const float distHigh = readParam ("distHigh");
    const int   rateIdx  = (int) readParam ("lfoRate");
    const float lfoVowel = readParam ("lfoVowel");
    const float lfoComb  = readParam ("lfoComb");
    const float lfoDrive = readParam ("lfoDrive");
    const float outGainDb = readParam ("outGain");
    const float ceilingDb = readParam ("ceiling");
    const float mix      = readParam ("mix");

    // ---- transport ----
    double bpm = 140.0, ppq = 0.0;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            if (auto q = pos->getPpqPosition()) ppq = *q;
            playing = pos->getIsPlaying();
        }

    static constexpr double cycleBeats[] = { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125 };
    const double beats = cycleBeats[juce::jlimit (0, 5, rateIdx)];
    const double phaseInc = (bpm / 60.0 / sr) / beats;

    juce::dsp::AudioBlock<float> block (buffer);
    auto io = block.getSubsetChannelBlock (0, (size_t) numCh);

    mixer.setWetMixProportion (mix);
    mixer.pushDrySamples (io);

    outSmooth.setTargetValue (juce::Decibels::decibelsToGain (outGainDb));

    // ---- stage 1: drive -> formants -> comb (base rate, modulated in sub-blocks) ----
    for (int start = 0; start < numSamples; start += kSubBlock)
    {
        const int len = juce::jmin (kSubBlock, numSamples - start);

        double phase = playing ? std::fmod (ppq / beats + start * phaseInc, 1.0) : lfoPhase;
        if (phase < 0.0) phase += 1.0;
        const bool wrapped = phase < lastPhase;
        lastPhase = phase;
        const float lfo = lfoValue (phase, wrapped);
        if (! playing)
        {
            lfoPhase += phaseInc * len;
            if (lfoPhase >= 1.0) lfoPhase -= std::floor (lfoPhase);
        }

        const float vowelPos = juce::jlimit (0.0f, 1.0f, vowel + lfoVowel * lfo * 0.5f);
        const float driveDb  = juce::jlimit (0.0f, 36.0f, drive + growl * 10.0f + lfoDrive * lfo * 12.0f);
        const float note     = juce::jlimit (24.0f, 96.0f, combPitch + lfoComb * lfo * 12.0f);
        const float combHz   = 440.0f * std::pow (2.0f, (note - 69.0f) / 12.0f);
        const float combDelay = (float) sr / combHz;
        const float fMix     = juce::jmax (formMix, growl * 0.8f);

        // vowel morph
        const float pos = vowelPos * 4.0f;
        const int v0 = juce::jmin (3, (int) pos);
        const float vf = pos - (float) v0;
        for (int k = 0; k < kNumFormants; ++k)
        {
            const float hz = kVowels[v0][k] + vf * (kVowels[v0 + 1][k] - kVowels[v0][k]);
            formant[(size_t) k].setCutoffFrequency (juce::jlimit (60.0f, (float) (sr * 0.45), hz));
            formant[(size_t) k].setResonance (juce::jlimit (0.7f, 20.0f, formRes));
        }

        driveSmooth.setTargetValue (juce::Decibels::decibelsToGain (driveDb));

        for (int i = 0; i < len; ++i)
        {
            const float g = driveSmooth.getNextValue();

            for (int ch = 0; ch < numCh; ++ch)
            {
                float* d = buffer.getWritePointer (ch);
                float x = std::tanh (d[start + i] * g);

                float fsum = 0.0f;
                for (int k = 0; k < kNumFormants; ++k)
                    fsum += formant[(size_t) k].processSample (ch, x) * kFormantGain[k];
                float f = x * (1.0f - fMix) + fsum * fMix * 1.6f;

                const float yc = comb.process (ch, f, combDelay, combFb);
                const float wetC = yc * (1.0f - 0.5f * combFb);
                d[start + i] = f * (1.0f - combMix) + wetC * combMix;
            }
        }
    }

    // ---- stage 2: 3-band distortion at 2x oversampling ----
    if (distLow > 0.0001f || distMid > 0.0001f || distHigh > 0.0001f)
    {
        auto os = oversampler->processSamplesUp (io);
        const int osN = (int) os.getNumSamples();

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* d = os.getChannelPointer ((size_t) ch);
            for (int i = 0; i < osN; ++i)
            {
                float lo, rest, mid, hi;
                split1.processSample (ch, d[i], lo, rest);
                split2.processSample (ch, rest, mid, hi);
                d[i] = shape (lo, distLow) + shape (mid, distMid) + shape (hi, distHigh);
            }
        }
        oversampler->processSamplesDown (io);
    }
    else
    {
        // Keep latency identical whether or not distortion is active.
        auto os = oversampler->processSamplesUp (io);
        juce::ignoreUnused (os);
        oversampler->processSamplesDown (io);
    }

    // ---- stage 3: output gain + limiter ----
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outSmooth.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
            buffer.getWritePointer (ch)[i] *= g;
    }

    limiter.setThreshold (ceilingDb);
    limiter.process (juce::dsp::ProcessContextReplacing<float> (io));

    // The limiter has no lookahead, so catch any residual overshoot with a hard ceiling.
    const float ceilGain = juce::Decibels::decibelsToGain (ceilingDb);
    for (int ch = 0; ch < numCh; ++ch)
        juce::FloatVectorOperations::clip (buffer.getWritePointer (ch), buffer.getReadPointer (ch),
                                           -ceilGain, ceilGain, numSamples);

    mixer.mixWetSamples (io);
}

// ------------------------------------------------------------------------------------
juce::AudioProcessorEditor* GrowlForgeProcessor::createEditor()
{
    return new GrowlForgeEditor (*this);
}

void GrowlForgeProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void GrowlForgeProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrowlForgeProcessor();
}
