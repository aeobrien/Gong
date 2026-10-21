#include "IRWaveformComponent.h"

IRWaveformComponent::IRWaveformComponent()
{
}

void IRWaveformComponent::setIR(const juce::AudioBuffer<float>* ir, int sampleRate)
{
    irData = ir;
    irSampleRate = sampleRate;
    buildThumbnail();
    repaint();
}

void IRWaveformComponent::clear()
{
    irData = nullptr;
    irSampleRate = 0;
    thumbnailMinL.clear();
    thumbnailMaxL.clear();
    thumbnailMinR.clear();
    thumbnailMaxR.clear();
    repaint();
}

void IRWaveformComponent::buildThumbnail()
{
    thumbnailMinL.clear();
    thumbnailMaxL.clear();
    thumbnailMinR.clear();
    thumbnailMaxR.clear();

    if (irData == nullptr || irData->getNumSamples() == 0)
        return;

    int width = getWidth();
    if (width <= 0)
        width = 400; // Default width for initial build

    int numSamples = irData->getNumSamples();
    int numChannels = irData->getNumChannels();

    // Calculate samples per pixel
    int samplesPerPixel = std::max(1, numSamples / width);

    int numPixels = (numSamples + samplesPerPixel - 1) / samplesPerPixel;

    thumbnailMinL.resize(numPixels);
    thumbnailMaxL.resize(numPixels);

    bool hasStereo = (numChannels > 1);
    if (hasStereo)
    {
        thumbnailMinR.resize(numPixels);
        thumbnailMaxR.resize(numPixels);
    }

    // Build thumbnail by finding min/max for each pixel
    for (int pixel = 0; pixel < numPixels; ++pixel)
    {
        int startSample = pixel * samplesPerPixel;
        int endSample = std::min(startSample + samplesPerPixel, numSamples);

        float minL = 1.0f, maxL = -1.0f;
        float minR = 1.0f, maxR = -1.0f;

        for (int sample = startSample; sample < endSample; ++sample)
        {
            float sampleL = irData->getSample(0, sample);
            minL = std::min(minL, sampleL);
            maxL = std::max(maxL, sampleL);

            if (hasStereo)
            {
                float sampleR = irData->getSample(1, sample);
                minR = std::min(minR, sampleR);
                maxR = std::max(maxR, sampleR);
            }
        }

        thumbnailMinL[pixel] = minL;
        thumbnailMaxL[pixel] = maxL;

        if (hasStereo)
        {
            thumbnailMinR[pixel] = minR;
            thumbnailMaxR[pixel] = maxR;
        }
    }
}

void IRWaveformComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(backgroundColour);
    g.fillRect(bounds);

    // Center line
    float centerY = bounds.getCentreY();
    g.setColour(centerLineColour);
    g.drawHorizontalLine(static_cast<int>(centerY), bounds.getX(), bounds.getRight());

    if (irData == nullptr || thumbnailMinL.empty())
    {
        // No IR loaded - draw placeholder text
        g.setColour(juce::Colours::grey);
        g.setFont(juce::FontOptions(14.0f));
        g.drawText("No IR loaded", bounds, juce::Justification::centred, true);
        return;
    }

    int numPixels = static_cast<int>(thumbnailMinL.size());
    float pixelWidth = bounds.getWidth() / static_cast<float>(numPixels);
    float halfHeight = bounds.getHeight() * 0.5f;

    // Draw right channel first (behind left)
    if (!thumbnailMinR.empty())
    {
        g.setColour(waveformColourR);
        for (int pixel = 0; pixel < numPixels; ++pixel)
        {
            float x = bounds.getX() + pixel * pixelWidth;
            float minY = centerY - thumbnailMaxR[pixel] * halfHeight;
            float maxY = centerY - thumbnailMinR[pixel] * halfHeight;

            g.drawVerticalLine(static_cast<int>(x), minY, maxY);
        }
    }

    // Draw left channel
    g.setColour(waveformColour);
    for (int pixel = 0; pixel < numPixels; ++pixel)
    {
        float x = bounds.getX() + pixel * pixelWidth;
        float minY = centerY - thumbnailMaxL[pixel] * halfHeight;
        float maxY = centerY - thumbnailMinL[pixel] * halfHeight;

        g.drawVerticalLine(static_cast<int>(x), minY, maxY);
    }

    // Draw time info
    if (irSampleRate > 0)
    {
        float lengthSeconds = static_cast<float>(irData->getNumSamples()) / static_cast<float>(irSampleRate);
        juce::String timeText = juce::String(lengthSeconds, 2) + "s";

        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(juce::FontOptions(11.0f));
        g.drawText(timeText, bounds.reduced(4), juce::Justification::bottomRight, true);
    }
}

void IRWaveformComponent::resized()
{
    // Rebuild thumbnail when size changes
    if (irData != nullptr)
        buildThumbnail();
}
