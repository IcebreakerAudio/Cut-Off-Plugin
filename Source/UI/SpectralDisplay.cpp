#include "SpectralDisplay.h"
#include "../PluginProcessor.h"

// ======== Analyser ========

SpectralAnalyser::SpectralAnalyser (CutOffAudioProcessor& processorToUse)
    : juce::Thread ("Spectral Analyser"), processor (processorToUse)
{
    latestFrame.resize (scopeSize, 0.0f);
}

SpectralAnalyser::~SpectralAnalyser()
{
    stopThread (1000);
}

void SpectralAnalyser::prepare(double sampleRate, int fifoMaxSize)
{
    sourceSampleRate = static_cast<float>(sampleRate);
    fifoPullBuffer.resize (std::max(fifoMaxSize, fftSize) + 1, 0.0f);
    prepared = true;
    startThread(juce::Thread::Priority::normal);
}

void SpectralAnalyser::run()
{
    while (!threadShouldExit() && prepared)
    {
        pushNextSampleBlockIntoAnalyser();
        wait(15);
    }
}

void SpectralAnalyser::pushNextSampleBlockIntoAnalyser()
{
    const int numRead = processor.getDataFromFifo (fifoPullBuffer, totalSamplesRead);
    totalSamplesRead += numRead;

    if (totalSamplesRead >= fftSize)
    {
        std::fill (fftData.begin(), fftData.end(), 0.0f);
        std::copy (fifoPullBuffer.begin() + (totalSamplesRead - fftSize), fifoPullBuffer.begin() + totalSamplesRead, fftData.begin());
        const auto elapsedSeconds = (float) totalSamplesRead / sourceSampleRate;
        const auto decayFactor = std::exp (-elapsedSeconds / decayTimeSeconds);
        totalSamplesRead = 0;
        performFFTAndPublish (decayFactor);
    }

}

float SpectralAnalyser::midiNoteToFrequency (float noteNumber, float frequencyOfA) const noexcept
{
    return frequencyOfA * std::pow (2.0f, (noteNumber - 69.0f) / 12.0f);
}

float SpectralAnalyser::frequencyToMidiNote (float frequencyHz, float frequencyOfA) const noexcept
{
    return 69.0f + 12.0f * std::log2 (frequencyHz / frequencyOfA);
}

float SpectralAnalyser::frequencyToFftIndex (float frequencyHz) const noexcept
{
    return juce::jlimit (0.0f, (float) (fftSize / 2), frequencyHz * (float) fftSize / sourceSampleRate);
}

float SpectralAnalyser::getInterpolatedPeak (float fftIndex) const noexcept
{
    const auto index0 = (int) std::floor (fftIndex);
    const auto index1 = juce::jmin (index0 + 1, fftSize / 2);
    const auto frac   = fftIndex - (float) index0;
    return juce::jmap (frac, peakBins[(size_t) index0], peakBins[(size_t) index1]);
}

void SpectralAnalyser::performFFTAndPublish (float decayFactor)
{
    window.multiplyWithWindowingTable (fftData.data(), fftSize);
    forwardFFT.performFrequencyOnlyForwardTransform (fftData.data());

    for (int bin = 0; bin <= fftSize / 2; ++bin)
    {
        auto level = juce::jmap (juce::Decibels::gainToDecibels (fftData[(size_t) bin]) - fftScaler,
                                 mindB, maxdB, 0.0f, 1.0f);
        auto value = juce::jlimit (0.0f, 1.0f, level);

        if (value > peakBins[(size_t) bin])
            peakBins[(size_t) bin] = value;
        else
            peakBins[(size_t) bin] *= decayFactor;
    }

    for (int i = 0; i < scopeSize; ++i)
    {
        const auto note     = juce::jmap ((float) i, 0.0f, (float) (scopeSize - 1), minNote, maxNote);
        const auto fftIndex = frequencyToFftIndex (midiNoteToFrequency (note));
        rawFrame[(size_t) i] = getInterpolatedPeak (fftIndex);
    }

    const juce::ScopedLock sl (frameLock);

    for (int i = 0; i < scopeSize; ++i)
    {
        const auto left   = rawFrame[(size_t) juce::jmax (i - 1, 0)];
        const auto centre = rawFrame[(size_t) i];
        const auto right  = rawFrame[(size_t) juce::jmin (i + 1, scopeSize - 1)];
        latestFrame[i] = (left + 2.0f * centre + right) * 0.25f;
    }

    newFrameAvailable.store (true, std::memory_order_release);
}

void SpectralAnalyser::getCurrentFrame (std::vector<float>& dest)
{
    {
        const juce::ScopedLock sl (frameLock);
        dest = latestFrame;
    }
    newFrameAvailable.store (false, std::memory_order_release);
}

// ======== Display ========

void SpectralDisplay::resized()
{
    auto bounds = getLocalBounds().toFloat();
    gradientFill = juce::ColourGradient(gradientStart, bounds.getX(), bounds.getY() + (bounds.getHeight() * 0.2f), 
                                        gradientEnd,   bounds.getX(), bounds.getHeight(), false);
}

void SpectralDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    g.setColour(backgroundColour);
    g.fillRoundedRectangle(bounds, 8.0f);

    const auto width = bounds.getWidth();
    const auto height = bounds.getHeight();
    const auto left = bounds.getX();
    const auto right = bounds.getRight();
    const auto top = bounds.getY();
    const auto bottom = bounds.getBottom();

    juce::Path spectrumPath;
    spectrumPath.startNewSubPath (left, bottom);

    const auto size = scopeData.size();
    for (int i = 0; i < size; ++i)
    {
        auto x = left + ((float) i / (float) size) * width;
        auto y = bottom - (scopeData[i] * height);
        spectrumPath.lineTo (x, juce::jlimit(top, bottom, y));
    }

    spectrumPath.lineTo(right, bottom);
    spectrumPath.closeSubPath();

    juce::Path clipPath;
    clipPath.addRoundedRectangle (bounds, 8.0f);

    g.saveState();
    g.reduceClipRegion (clipPath);

    g.setGradientFill(gradientFill);
    g.fillPath(spectrumPath);

    g.setColour(topEdgeColour);
    g.strokePath(spectrumPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));

    g.restoreState();

    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
}
