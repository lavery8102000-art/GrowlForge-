#pragma once

#include "PluginProcessor.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

class GrowlForgeEditor : public juce::AudioProcessorEditor
{
public:
    explicit GrowlForgeEditor (GrowlForgeProcessor&);
    ~GrowlForgeEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    struct Group
    {
        juce::String title;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> bounds;
    };

    Knob& addKnob (const juce::String& paramId, const juce::String& text);

    GrowlForgeProcessor& proc;
    juce::LookAndFeel_V4 laf;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Group> groups;

    juce::ComboBox presetBox, rateBox, shapeBox;
    juce::Label rateLabel, shapeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> rateAttach, shapeAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrowlForgeEditor)
};
