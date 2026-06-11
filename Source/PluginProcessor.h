#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

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
    enum
    {
        fftOrder  = 11,
        fftSize   = 1 << fftOrder,
        scopeSize = 512
    };

    void pushNextSampleIntoFifo (float sample) noexcept;

    float fifo[fftSize];
    float fftData[2 * fftSize];
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // --- ADD THESE DSP OBJECTS ---
    juce::dsp::StateVariableTPTFilter<float> hpf;
    juce::dsp::StateVariableTPTFilter<float> lpf;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::Gain<float> outputGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CutOffAudioProcessor)
};