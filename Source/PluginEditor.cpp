#include "PluginEditor.h"
#include "BinaryData.h"

void CutOffAudioProcessorEditor::setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& name)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    slider.setLookAndFeel(&glowLookAndFeel);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setFont(juce::Font(14.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false);
    addAndMakeVisible(label);
}

CutOffAudioProcessorEditor::CutOffAudioProcessorEditor (CutOffAudioProcessor& p)
    : AudioProcessorEditor (&p), 
      forwardFFT (CutOffAudioProcessor::fftOrder),
      window (CutOffAudioProcessor::fftSize, juce::dsp::WindowingFunction<float>::hann),
      audioProcessor (p)
{
    setSize (700, 450);
    
    // Clear visualizer memory
    juce::zeromem(scopeData, sizeof(scopeData));

    // Safely load both SVG Logos from BinaryData using the exact filenames
    cutOffLogo = juce::Drawable::createFromImageData(BinaryData::CutOffLogo_svg, BinaryData::CutOffLogo_svgSize);
    lineageLogo = juce::Drawable::createFromImageData(BinaryData::LineageLogo_svg, BinaryData::LineageLogo_svgSize);

    // ==============================================================================
    // 1. SETUP UI STYLES (Applies the neon vector glow to ALL knobs)
    // ==============================================================================
    setupSlider(hpfSlider, hpfLabel, "HPF");
    setupSlider(lpfSlider, lpfLabel, "LPF");
    setupSlider(attackSlider, attackLabel, "ATTACK");
    setupSlider(decaySlider, decayLabel, "DECAY");
    setupSlider(thresholdSlider, thresholdLabel, "THRESHOLD");
    setupSlider(gainSlider, gainLabel, "GAIN");

    // ==============================================================================
    // 2. ATTACH KNOBS TO DSP (Ensures turning the knobs actually filters the audio)
    // ==============================================================================
    hpfAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "HPF", hpfSlider);
    lpfAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "LPF", lpfSlider);
    atkAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "ATTACK", attackSlider);
    decAttach    = std::make_unique<SliderAttachment>(audioProcessor.apvts, "DECAY", decaySlider);
    threshAttach = std::make_unique<SliderAttachment>(audioProcessor.apvts, "THRESHOLD", thresholdSlider);
    gainAttach   = std::make_unique<SliderAttachment>(audioProcessor.apvts, "GAIN", gainSlider);

    startTimerHz(60);
}

CutOffAudioProcessorEditor::~CutOffAudioProcessorEditor()
{
    hpfSlider.setLookAndFeel(nullptr);
    lpfSlider.setLookAndFeel(nullptr);
    attackSlider.setLookAndFeel(nullptr);
    decaySlider.setLookAndFeel(nullptr);
    thresholdSlider.setLookAndFeel(nullptr);
    gainSlider.setLookAndFeel(nullptr);
}

void CutOffAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.nextFFTBlockReady)
    {
        drawNextFrameOfSpectrum();
        audioProcessor.nextFFTBlockReady = false;
        repaint(); // Tells JUCE to call paint() again
    }
}

void CutOffAudioProcessorEditor::drawNextFrameOfSpectrum()
{
    // Apply windowing and perform the Fast Fourier Transform
    window.multiplyWithWindowingTable (audioProcessor.fftData, CutOffAudioProcessor::fftSize);
    forwardFFT.performFrequencyOnlyForwardTransform (audioProcessor.fftData);

    auto mindB = -100.0f;
    auto maxdB =    0.0f;

    for (int i = 0; i < CutOffAudioProcessor::scopeSize; ++i)
    {
        // Skew the x-axis to be logarithmic (like human hearing)
        auto skewedProportionX = 1.0f - std::exp (std::log (1.0f - (float) i / (float) CutOffAudioProcessor::scopeSize) * 0.2f);
        auto fftDataIndex = juce::jlimit (0, CutOffAudioProcessor::fftSize / 2, (int) (skewedProportionX * (float) CutOffAudioProcessor::fftSize * 0.5f));
        auto level = juce::jmap (juce::Decibels::gainToDecibels (audioProcessor.fftData[fftDataIndex])
                               - juce::Decibels::gainToDecibels ((float) CutOffAudioProcessor::fftSize),
                                 mindB, maxdB, 0.0f, 1.0f);

        scopeData[i] = level;
    }
}

void CutOffAudioProcessorEditor::paint (juce::Graphics& g)
{
    // 1. Deeply dark charcoal background
    g.fillAll (juce::Colour(0xff0D0D0D));

    // 2. Center the Cut-Off Logo in the top area
    if (cutOffLogo != nullptr)
    {
        juce::Rectangle<float> cutOffBounds(0, 10, getWidth(), 80);
        cutOffLogo->drawWithin(g, cutOffBounds, juce::RectanglePlacement::centred, 1.0f);
    }

    // Right-align the Lineage Audio Labs Logo
    if (lineageLogo != nullptr)
    {
        juce::Rectangle<float> lineageBounds(getWidth() - 140, 15, 120, 45);
        lineageLogo->drawWithin(g, lineageBounds, juce::RectanglePlacement::xRight | juce::RectanglePlacement::yMid, 1.0f);
    }

    // 3. Centerpiece Display (Spectrum Visualizer)
    juce::Rectangle<float> displayArea(180.0f, 110.0f, 340.0f, 150.0f);
    
    // Background of visualizer
    g.setColour(juce::Colour(0xff151515));
    g.fillRoundedRectangle(displayArea, 8.0f);

    // ==============================================================================
    // FIX: Using a safe C++ scope block { } to manage the graphics state
    // ==============================================================================
    {
        juce::Graphics::ScopedSaveState state(g);
        g.reduceClipRegion(displayArea.toNearestInt()); // Keep drawing strictly inside the box

        juce::Path spectrumPath;
        auto width = displayArea.getWidth();
        auto height = displayArea.getHeight();
        auto bottom = displayArea.getBottom();
        auto left = displayArea.getX();

        spectrumPath.startNewSubPath (left, bottom);

        for (int i = 0; i < CutOffAudioProcessor::scopeSize; ++i)
        {
            auto x = left + ((float) i / (float) CutOffAudioProcessor::scopeSize) * width;
            auto y = bottom - (scopeData[i] * height);
            spectrumPath.lineTo (x, juce::jlimit(displayArea.getY(), bottom, y));
        }
        
        spectrumPath.lineTo(displayArea.getRight(), bottom);
        spectrumPath.closeSubPath();

        // Fill with glowing neon cyan gradient
        juce::Colour gradientStart = juce::Colour(0x9900E5FF); 
        juce::Colour gradientEnd = juce::Colour(0x0000E5FF);   
        g.setGradientFill(juce::ColourGradient(gradientStart, displayArea.getX(), displayArea.getY() + (height * 0.2f), 
                                               gradientEnd, displayArea.getX(), bottom, false));
        g.fillPath(spectrumPath);
        
        // Draw the crisp top edge line
        g.setColour(juce::Colour(0xff00E5FF));
        g.strokePath(spectrumPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved));
    } // <--- The graphics state safely and automatically restores exactly here!
    // ==============================================================================

    // Draw the outer border (now safely outside the clipping region)
    g.setColour(juce::Colour(0xff2A2A2A));
    g.drawRoundedRectangle(displayArea, 8.0f, 2.0f);
}

void CutOffAudioProcessorEditor::resized()
{
    const int knobSize = 100; 
    
    hpfSlider.setBounds (40, 140, knobSize, knobSize);
    lpfSlider.setBounds (560, 140, knobSize, knobSize);
    
    attackSlider.setBounds (40, 310, knobSize, knobSize);
    decaySlider.setBounds (170, 310, knobSize, knobSize);
    thresholdSlider.setBounds (300, 310, knobSize, knobSize);
    gainSlider.setBounds (560, 310, knobSize, knobSize);
}