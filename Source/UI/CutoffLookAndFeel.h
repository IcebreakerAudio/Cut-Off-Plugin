#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

class CutoffLookAndFeel : public juce::LookAndFeel_V4
{
public:
    CutoffLookAndFeel() {
        setColour(juce::Label::textColourId, juce::Colours::white);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, const float rotaryStartAngle, 
                           const float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float) juce::jmin (width / 2, height / 2) - 8.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colour(0xff262626));
        g.strokePath(backgroundArc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (sliderPos > 0.0f) {
            juce::Path fillArc;
            fillArc.addCentredArc(centreX, centreY, radius, radius, 0.0f, rotaryStartAngle, angle, true);
            juce::DropShadow glow(juce::Colour(0xff00E5FF).withAlpha(0.6f), 8, {0, 0});
            glow.drawForPath(g, fillArc);
            g.setColour(juce::Colour(0xff00E5FF));
            g.strokePath(fillArc, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        g.setColour(juce::Colour(0xff1F1F1F));
        g.fillEllipse(rx + 4.0f, ry + 4.0f, rw - 8.0f, rw - 8.0f);
        g.setColour(juce::Colour(0xff111111));
        g.fillEllipse(rx + 6.0f, ry + 6.0f, rw - 12.0f, rw - 12.0f);

        juce::Path pointer;
        pointer.addRoundedRectangle(-2.0f, -radius + 12.0f, 4.0f, radius * 0.6f, 2.0f);
        pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.setColour(juce::Colours::white);
        g.fillPath(pointer);
    }
};
