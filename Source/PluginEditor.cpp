#include "PluginEditor.h"

namespace
{
    const juce::Colour kBg      { 0xff120b0e };
    const juce::Colour kPanel   { 0xff1e1317 };
    const juce::Colour kAccent  { 0xffd8324a };
    const juce::Colour kText    { 0xffe9dde0 };
    const juce::Colour kDim     { 0xff9a8a8f };
}

GrowlForgeEditor::GrowlForgeEditor (GrowlForgeProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    laf.setColour (juce::Slider::rotarySliderFillColourId, kAccent);
    laf.setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (0xff3a2830));
    laf.setColour (juce::Slider::thumbColourId, kText);
    laf.setColour (juce::Slider::textBoxTextColourId, kText);
    laf.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    laf.setColour (juce::Label::textColourId, kDim);
    laf.setColour (juce::ComboBox::backgroundColourId, kPanel);
    laf.setColour (juce::ComboBox::textColourId, kText);
    laf.setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff3a2830));
    laf.setColour (juce::ComboBox::arrowColourId, kAccent);
    laf.setColour (juce::PopupMenu::backgroundColourId, kPanel);
    laf.setColour (juce::PopupMenu::textColourId, kText);
    laf.setColour (juce::PopupMenu::highlightedBackgroundColourId, kAccent);
    setLookAndFeel (&laf);

    auto k = [this] (const char* id, const char* text) -> Knob* { return &addKnob (id, text); };

    groups.push_back ({ "INPUT",   { k ("drive", "Drive"), k ("growl", "Growl") }, {} });
    groups.push_back ({ "FORMANT", { k ("vowel", "Vowel"), k ("formRes", "Reso"), k ("formMix", "Mix") }, {} });
    groups.push_back ({ "COMB",    { k ("combPitch", "Pitch"), k ("combFb", "Feedback"), k ("combMix", "Mix") }, {} });
    groups.push_back ({ "DISTORTION", { k ("distLow", "Low"), k ("distMid", "Mid"), k ("distHigh", "High") }, {} });
    groups.push_back ({ "LFO",     { k ("lfoVowel", "> Vowel"), k ("lfoComb", "> Comb"), k ("lfoDrive", "> Drive") }, {} });
    groups.push_back ({ "OUTPUT",  { k ("outGain", "Level"), k ("ceiling", "Ceiling"), k ("mix", "Mix") }, {} });

    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    rateBox.addItemList (GrowlForgeProcessor::lfoRateNames(), 1);
    shapeBox.addItemList (GrowlForgeProcessor::lfoShapeNames(), 1);
    addAndMakeVisible (rateBox);
    addAndMakeVisible (shapeBox);
    rateAttach  = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "lfoRate", rateBox);
    shapeAttach = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "lfoShape", shapeBox);

    for (auto* l : { &rateLabel, &shapeLabel })
    {
        l->setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (l);
    }
    rateLabel.setText ("Rate", juce::dontSendNotification);
    shapeLabel.setText ("Shape", juce::dontSendNotification);

    setSize (780, 480);
}

GrowlForgeEditor::~GrowlForgeEditor()
{
    setLookAndFeel (nullptr);
}

GrowlForgeEditor::Knob& GrowlForgeEditor::addKnob (const juce::String& paramId, const juce::String& text)
{
    auto kn = std::make_unique<Knob>();
    kn->slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    kn->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
    kn->slider.setDoubleClickReturnValue (true, proc.apvts.getParameter (paramId)->convertFrom0to1 (
                                                    proc.apvts.getParameter (paramId)->getDefaultValue()));
    kn->label.setText (text, juce::dontSendNotification);
    kn->label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (kn->slider);
    addAndMakeVisible (kn->label);
    kn->attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, kn->slider);

    knobs.push_back (std::move (kn));
    return *knobs.back();
}

void GrowlForgeEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    g.setColour (kAccent);
    g.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    g.drawText ("GROWLFORGE", 20, 12, 300, 36, juce::Justification::centredLeft);

    g.setColour (kDim);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    g.drawText ("bass design processor  -  put it after any synth", 20, 44, 400, 16, juce::Justification::centredLeft);

    for (auto& grp : groups)
    {
        g.setColour (kPanel);
        g.fillRoundedRectangle (grp.bounds.toFloat(), 8.0f);
        g.setColour (kAccent);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText (grp.title, grp.bounds.withHeight (22).withTrimmedLeft (12), juce::Justification::centredLeft);
    }
}

void GrowlForgeEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    auto header = area.removeFromTop (48);
    presetBox.setBounds (header.removeFromRight (200).withTrimmedTop (8).withHeight (28));
    area.removeFromTop (8);

    const int rows = 2, cols = 3, gap = 10;
    const int cellW = (area.getWidth() - gap * (cols - 1)) / cols;
    const int cellH = (area.getHeight() - gap * (rows - 1)) / rows;

    for (size_t i = 0; i < groups.size(); ++i)
    {
        const int r = (int) i / cols, c = (int) i % cols;
        auto cell = juce::Rectangle<int> (area.getX() + c * (cellW + gap), area.getY() + r * (cellH + gap), cellW, cellH);
        groups[i].bounds = cell;

        auto inner = cell.reduced (8);
        inner.removeFromTop (20);

        const bool isLfo = groups[i].title == "LFO";
        if (isLfo)
        {
            auto top = inner.removeFromTop (28);
            rateLabel.setBounds (top.removeFromLeft (34));
            rateBox.setBounds (top.removeFromLeft (64).reduced (0, 2));
            shapeLabel.setBounds (top.removeFromLeft (44));
            shapeBox.setBounds (top.reduced (0, 2));
        }

        const int n = (int) groups[i].knobs.size();
        const int kw = inner.getWidth() / n;
        for (int j = 0; j < n; ++j)
        {
            auto slot = inner.removeFromLeft (kw);
            auto* kn = groups[i].knobs[(size_t) j];
            kn->label.setBounds (slot.removeFromTop (16));
            kn->slider.setBounds (slot);
        }
    }
}
