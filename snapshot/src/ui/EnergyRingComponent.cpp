#include "EnergyRingComponent.h"

EnergyRingComponent::EnergyRingComponent()
{
    setOpaque(false);
}

void EnergyRingComponent::setGlobalEnergy(float energy)
{
    globalEnergy = energy;
    repaint();
}

void EnergyRingComponent::setBandEnergy(int band, float energy)
{
    if (band >= 0 && band < 4)
    {
        bandEnergies[band] = energy;
        repaint();
    }
}

void EnergyRingComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(4.0f);
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto maxRadius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;

    // Outer ring background
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillEllipse(cx - maxRadius, cy - maxRadius, maxRadius * 2.0f, maxRadius * 2.0f);

    // 4 concentric arcs for band energies (outermost = band 3/high, innermost = band 0/low)
    float ringWidth = 4.0f;
    float ringGap = 2.0f;
    float startAngle = -juce::MathConstants<float>::halfPi; // 12 o'clock

    for (int i = 3; i >= 0; --i)
    {
        float radius = maxRadius - (3 - i) * (ringWidth + ringGap) - 2.0f;
        if (radius <= 0.0f) continue;

        float energy = juce::jlimit(0.0f, 1.0f, bandEnergies[i]);
        float sweepAngle = energy * juce::MathConstants<float>::twoPi;

        if (sweepAngle > 0.01f)
        {
            juce::Path arc;
            arc.addCentredArc(cx, cy, radius, radius, 0.0f,
                              startAngle, startAngle + sweepAngle, true);
            g.setColour(juce::Colour(kBandColours[i]));
            g.strokePath(arc, juce::PathStrokeType(ringWidth, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        }

        // Track ring (dim)
        juce::Path trackArc;
        trackArc.addCentredArc(cx, cy, radius, radius, 0.0f,
                               startAngle, startAngle + juce::MathConstants<float>::twoPi - 0.01f, true);
        g.setColour(juce::Colour(kBandColours[i]).withAlpha(0.15f));
        g.strokePath(trackArc, juce::PathStrokeType(ringWidth, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }

    // Center circle for global energy
    float centerRadius = maxRadius - 4 * (ringWidth + ringGap) - 4.0f;
    if (centerRadius > 2.0f)
    {
        float gEnergy = juce::jlimit(0.0f, 1.0f, globalEnergy);

        // Background
        g.setColour(juce::Colour(0xff252525));
        g.fillEllipse(cx - centerRadius, cy - centerRadius,
                       centerRadius * 2.0f, centerRadius * 2.0f);

        // Filled proportional to energy
        auto energyColour = juce::Colour(0xffff9800).interpolatedWith(
            juce::Colour(0xfff44336), gEnergy);
        float fillRadius = centerRadius * gEnergy;
        g.setColour(energyColour.withAlpha(0.8f));
        g.fillEllipse(cx - fillRadius, cy - fillRadius,
                       fillRadius * 2.0f, fillRadius * 2.0f);

        // Text
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText(juce::String(static_cast<int>(gEnergy * 100)) + "%",
                   juce::Rectangle<float>(cx - centerRadius, cy - 6, centerRadius * 2.0f, 12.0f),
                   juce::Justification::centred);
    }
}
