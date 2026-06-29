#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "UI/SpectralDisplay.h"

class CutOffAudioProcessor  : public juce::AudioProcessor
{
public:
    CutOffAudioProcessor();
    ~CutOffAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // reads from fifo into a vector and returns size of data read
    int getDataFromFifo(std::vector<float>& output, const int indexOffset);
    bool getScopeData(std::vector<float>& output);

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AbstractFifo fifo {1};
    void pushBufferToFifo (const juce::AudioBuffer<float>& buffer);
    std::vector<float> fifoData;
    SpectralAnalyser spectralAnalyser;

    juce::dsp::StateVariableTPTFilter<float> hpf, lpf;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Gain<float> outputGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CutOffAudioProcessor)
};