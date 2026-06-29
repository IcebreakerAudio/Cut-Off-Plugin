#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>

class CutOffAudioProcessor;

class SpectralAnalyser : public juce::Thread
{
public:
    explicit SpectralAnalyser (CutOffAudioProcessor& processorToUse);
    ~SpectralAnalyser() override;

    void run() override;

    static constexpr int scopeSize = 512;

    void prepare(double sampleRate, int fifoMaxSize);

    bool isNewFrameAvailable() const noexcept { return newFrameAvailable.load (std::memory_order_acquire); }

    void getCurrentFrame (std::vector<float>& dest);

private:
    void pushNextSampleBlockIntoAnalyser();
    void performFFTAndPublish (float decayFactor);

    float midiNoteToFrequency (float noteNumber, float frequencyOfA = 440.0f) const noexcept;
    float frequencyToMidiNote (float frequencyHz, float frequencyOfA = 440.0f) const noexcept;
    float frequencyToFftIndex (float frequencyHz) const noexcept;
    float getInterpolatedPeak (float fftIndex) const noexcept;

    CutOffAudioProcessor& processor;

    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;

    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float mindB   = -96.0f;
    static constexpr float maxdB   = 0.0f;
    static constexpr float decayTimeSeconds = 0.3f;
    float sourceSampleRate = 44100.0f;
    bool prepared = false;

    const float minNote = frequencyToMidiNote (minFreq);
    const float maxNote = frequencyToMidiNote (maxFreq);
    const float fftScaler = juce::Decibels::gainToDecibels ((float) fftSize);

    std::array<float, fftSize>   inputData {0.0f};
    std::array<float, 2 * fftSize> fftData {0.0f};
    std::array<float, (fftSize / 2) + 1> peakBins {0.0f};
    std::array<float, scopeSize> rawFrame {};

    juce::dsp::FFT forwardFFT {fftOrder};
    juce::dsp::WindowingFunction<float> window {fftSize, juce::dsp::WindowingFunction<float>::hann};

    std::vector<float> fifoPullBuffer;
    int totalSamplesRead = 0;

    juce::CriticalSection frameLock;
    std::vector<float> latestFrame;
    std::atomic<bool> newFrameAvailable {false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralAnalyser)
};

class SpectralDisplay : public juce::Component
{
public:
    SpectralDisplay() { scopeData.resize (SpectralAnalyser::scopeSize, 0.0f); }

    void setScopeData (const std::vector<float>& newData)
    {
        scopeData = newData;
        repaint();
    }

    std::vector<float>& data() { return scopeData; }

    void resized() override;
    void paint(juce::Graphics& g) override;

private:

    const juce::Colour backgroundColour {0xff151515};
    const juce::Colour topEdgeColour    {0xff00E5FF};
    const juce::Colour borderColour     {0xff2A2A2A};
    const juce::Colour gradientStart    {0x9900E5FF};
    const juce::Colour gradientEnd      {0x0000E5FF};
    juce::ColourGradient gradientFill;

    std::vector<float> scopeData;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectralDisplay)
};
