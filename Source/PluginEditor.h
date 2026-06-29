#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "UI/CutoffLookAndFeel.h"
#include "PluginProcessor.h"
#include "UI/SpectralDisplay.h"

class CutOffAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    CutOffAudioProcessorEditor (CutOffAudioProcessor&);
    ~CutOffAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    CutOffAudioProcessor& audioProcessor;
    GlowLookAndFeel glowLookAndFeel;

    SpectralDisplay spectralDisplay;
    
    std::unique_ptr<juce::Drawable> cutOffLogo;
    std::unique_ptr<juce::Drawable> lineageLogo;

    juce::Slider hpfSlider, lpfSlider, attackSlider, decaySlider, thresholdSlider, gainSlider;
    juce::Label hpfLabel, lpfLabel, attackLabel, decayLabel, thresholdLabel, gainLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> hpfAttach, lpfAttach, atkAttach, decAttach, threshAttach, gainAttach;

    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& name);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CutOffAudioProcessorEditor)
};