#include "PluginEditor.h"
#include "BinaryData.h"

void CutOffAudioProcessorEditor::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& name, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    slider.setTextValueSuffix(suffix);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setFont(juce::Font(14.0f, juce::Font::bold));
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false);
    addAndMakeVisible(label);
}

CutOffAudioProcessorEditor::CutOffAudioProcessorEditor (CutOffAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p)
{
    
    cutOffLogo  = juce::Drawable::createFromImageData(BinaryData::CutOffLogo_svg,  BinaryData::CutOffLogo_svgSize);
    lineageLogo = juce::Drawable::createFromImageData(BinaryData::LineageLogo_svg, BinaryData::LineageLogo_svgSize);
    addAndMakeVisible(cutOffLogo.get());
    addAndMakeVisible(lineageLogo.get());

    setupSlider(hpfSlider, hpfLabel, "HPF", " Hz");
    setupSlider(lpfSlider, lpfLabel, "LPF", " Hz");
    setupSlider(attackSlider, attackLabel, "ATTACK", " ms");
    setupSlider(decaySlider, decayLabel, "DECAY", " ms");
    setupSlider(thresholdSlider, thresholdLabel, "THRESHOLD", " dB");
    setupSlider(gainSlider, gainLabel, "GAIN", " dB");

    hpfAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "HPF", hpfSlider);
    lpfAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "LPF", lpfSlider);
    atkAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "ATTACK", attackSlider);
    decAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "DECAY", decaySlider);
    threshAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "THRESHOLD", thresholdSlider);
    gainAttach   = std::make_unique<SliderAttachment>(audioProcessor.apvts, "GAIN", gainSlider);

    addAndMakeVisible (spectralDisplay);
    
    setLookAndFeel(&lookAndFeel);
    setSize (700, 450);

    startTimerHz(60);
}

CutOffAudioProcessorEditor::~CutOffAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void CutOffAudioProcessorEditor::timerCallback()
{
    auto newData = audioProcessor.getScopeData(spectralDisplay.data());
    if(newData) spectralDisplay.repaint();
}

void CutOffAudioProcessorEditor::paint (juce::Graphics& g)
{
    // 1. Deeply dark charcoal background
    g.fillAll (juce::Colour(0xff0D0D0D));
}

void CutOffAudioProcessorEditor::resized()
{
    const int knobSize = 100;

    // Center the Cut-Off Logo in the top area
    if (cutOffLogo != nullptr)
    {
        juce::Rectangle<float> cutOffBounds(0, 10, getWidth(), 80);
        cutOffLogo->setTransformToFit(cutOffBounds, juce::RectanglePlacement::centred);
    }

    // Right-align the Lineage Audio Labs Logo
    if (lineageLogo != nullptr)
    {
        juce::Rectangle<float> lineageBounds(getWidth() - 140, 15, 120, 45);
        lineageLogo->setTransformToFit(lineageBounds, juce::RectanglePlacement::xRight | juce::RectanglePlacement::yMid);
    }
    
    hpfSlider.setBounds (40, 140, knobSize, knobSize);
    lpfSlider.setBounds (560, 140, knobSize, knobSize);
    
    attackSlider.setBounds (40, 310, knobSize, knobSize);
    decaySlider.setBounds (170, 310, knobSize, knobSize);
    thresholdSlider.setBounds (300, 310, knobSize, knobSize);
    gainSlider.setBounds (560, 310, knobSize, knobSize);

    spectralDisplay.setBounds (180, 110, 340, 150);
}