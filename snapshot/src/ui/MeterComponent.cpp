#include "MeterComponent.h"

MeterComponent::MeterComponent()
{
    setOpaque(false);
}

void MeterComponent::setMeter(const StageMeter* meter)
{
    source = meter;
}

void MeterComponent::setColour(juce::Colour colour)
{
    barColour = colour;
}

void MeterComponent::refresh()
{
    if (source == nullptr) return;

    rmsL = source->rmsL.load(std::memory_order_relaxed);
    rmsR = source->rmsR.load(std::memory_order_relaxed);
    peakL = source->peakL.load(std::memory_order_relaxed);
    peakR = source->peakR.load(std::memory_order_relaxed);

    // Peak hold with decay
    if (peakL > peakHoldL) peakHoldL = peakL;
    else peakHoldL *= kPeakDecay;
    if (peakR > peakHoldR) peakHoldR = peakR;
    else peakHoldR *= kPeakDecay;

    repaint();
}

void MeterComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    bool isVertical = bounds.getHeight() > bounds.getWidth();

    if (isVertical)
    {
        float halfW = bounds.getWidth() / 2.0f - 1.0f;
        auto leftBar = bounds.removeFromLeft(halfW);
        bounds.removeFromLeft(2.0f);
        auto rightBar = bounds;

        // Background
        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(leftBar);
        g.fillRect(rightBar);

        // RMS bars (from bottom)
        float rmsLH = juce::jlimit(0.0f, 1.0f, rmsL) * leftBar.getHeight();
        float rmsRH = juce::jlimit(0.0f, 1.0f, rmsR) * rightBar.getHeight();

        g.setColour(barColour);
        g.fillRect(leftBar.removeFromBottom(rmsLH));

        auto rbounds = rightBar;
        g.fillRect(rbounds.removeFromBottom(rmsRH));

        // Peak indicators
        g.setColour(barColour.brighter(0.5f));
        float peakLY = leftBar.getBottom() - peakHoldL * getHeight();
        float peakRY = rightBar.getBottom() - peakHoldR * getHeight();
        g.fillRect(leftBar.getX(), peakLY, halfW, 1.5f);
        g.fillRect(rightBar.getX(), peakRY, halfW, 1.5f);
    }
    else
    {
        float halfH = bounds.getHeight() / 2.0f - 1.0f;
        auto topBar = bounds.removeFromTop(halfH);
        bounds.removeFromTop(2.0f);
        auto bottomBar = bounds;

        g.setColour(juce::Colour(0xff1a1a1a));
        g.fillRect(topBar);
        g.fillRect(bottomBar);

        float rmsLW = juce::jlimit(0.0f, 1.0f, rmsL) * topBar.getWidth();
        float rmsRW = juce::jlimit(0.0f, 1.0f, rmsR) * bottomBar.getWidth();

        g.setColour(barColour);
        g.fillRect(topBar.removeFromLeft(rmsLW));
        g.fillRect(bottomBar.removeFromLeft(rmsRW));
    }
}
