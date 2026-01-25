#include "Resonator.h"

constexpr float Resonator::kGongModeRatios[];
constexpr float Resonator::kDefaultModeDecayMultipliers[];

Resonator::Resonator()
{
    modeGains.fill(1.0f);
    modeFreqL.fill(220.0f);
    modeFreqR.fill(220.0f);

    // Initialize per-mode params with defaults
    for (int i = 0; i < kMaxModes; ++i)
    {
        modeParams[i] = ModeParams();
        // Disable higher modes by default
        modeParams[i].enabled = (i < 6);
    }
}

void Resonator::prepare(double newSampleRate, int newBlockSize)
{
    sampleRate = newSampleRate;
    blockSize = newBlockSize;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    for (int i = 0; i < kMaxModes; ++i)
    {
        filtersL[i].prepare(spec);
        filtersR[i].prepare(spec);
    }

    needsUpdate = true;
    updateFilters();
}

void Resonator::process(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output)
{
    if (needsUpdate)
        updateFilters();

    auto numSamples = input.getNumSamples();
    auto numChannels = juce::jmin(input.getNumChannels(), output.getNumChannels());

    output.clear();

    // Output gain boost to compensate for high-Q bandpass filter attenuation
    constexpr float outputGainBoost = 8.0f;

    // Apply master gain (dB to linear)
    float masterGainLinear = juce::Decibels::decibelsToGain(masterGainDb);

    // Process each active mode
    for (int mode = 0; mode < numActiveModes; ++mode)
    {
        if (!modeParams[mode].enabled)
            continue;

        float modeFreq = getEffectiveModeFrequency(mode);

        // Skip if mode frequency is too high (above Nyquist * 0.9)
        if (modeFreq > sampleRate * 0.45f)
            continue;

        float gain = modeGains[mode] * outputGainBoost * masterGainLinear;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Left channel
            if (numChannels > 0)
            {
                float inSample = input.getSample(0, sample);
                float filtered = filtersL[mode].processSample(inSample);
                output.addSample(0, sample, filtered * gain);
            }

            // Right channel
            if (numChannels > 1)
            {
                float inSample = input.getSample(1, sample);
                float filtered = filtersR[mode].processSample(inSample);
                output.addSample(1, sample, filtered * gain);
            }
        }
    }
}

void Resonator::reset()
{
    for (int i = 0; i < kMaxModes; ++i)
    {
        filtersL[i].reset();
        filtersR[i].reset();
    }
}

void Resonator::setFundamentalFrequency(float frequencyHz)
{
    if (fundamentalFrequency != frequencyHz)
    {
        fundamentalFrequency = juce::jlimit(20.0f, 2000.0f, frequencyHz);
        needsUpdate = true;
    }
}

void Resonator::setDecayTime(float decaySeconds)
{
    if (decayTime != decaySeconds)
    {
        decayTime = juce::jlimit(0.1f, 30.0f, decaySeconds);
        needsUpdate = true;
    }
}

void Resonator::setBrightness(float newBrightness)
{
    if (brightness != newBrightness)
    {
        brightness = juce::jlimit(0.0f, 1.0f, newBrightness);
        needsUpdate = true;
    }
}

void Resonator::setNumActiveModes(int numModes)
{
    numActiveModes = juce::jlimit(1, kMaxModes, numModes);
}

void Resonator::setMasterGainDb(float gainDb)
{
    masterGainDb = juce::jlimit(-24.0f, 24.0f, gainDb);
}

const Resonator::ModeParams& Resonator::getModeParams(int mode) const
{
    return modeParams[juce::jlimit(0, kMaxModes - 1, mode)];
}

void Resonator::setModeParams(int mode, const ModeParams& params)
{
    if (mode >= 0 && mode < kMaxModes)
    {
        modeParams[mode] = params;
        needsUpdate = true;
    }
}

void Resonator::setModeFrequencyRatio(int mode, float ratio)
{
    if (mode >= 0 && mode < kMaxModes)
    {
        modeParams[mode].frequencyRatio = juce::jlimit(0.5f, 2.0f, ratio);
        needsUpdate = true;
    }
}

void Resonator::setModeDecayMultiplier(int mode, float multiplier)
{
    if (mode >= 0 && mode < kMaxModes)
    {
        modeParams[mode].decayMultiplier = juce::jlimit(0.1f, 3.0f, multiplier);
        needsUpdate = true;
    }
}

void Resonator::setModeGainDb(int mode, float gainDb)
{
    if (mode >= 0 && mode < kMaxModes)
    {
        modeParams[mode].gainDb = juce::jlimit(-24.0f, 6.0f, gainDb);
        needsUpdate = true;
    }
}

void Resonator::setModeDetuneCents(int mode, float cents)
{
    if (mode >= 0 && mode < kMaxModes)
    {
        modeParams[mode].detuneCents = juce::jlimit(-100.0f, 100.0f, cents);
        needsUpdate = true;
    }
}

void Resonator::setModeStereoWidth(int mode, float width)
{
    if (mode >= 0 && mode < kMaxModes)
    {
        modeParams[mode].stereoWidth = juce::jlimit(0.0f, 1.0f, width);
        needsUpdate = true;
    }
}

void Resonator::setModeEnabled(int mode, bool enabled)
{
    if (mode >= 0 && mode < kMaxModes)
    {
        modeParams[mode].enabled = enabled;
    }
}

float Resonator::getBaseModeRatio(int mode) const
{
    if (mode >= 0 && mode < kMaxModes)
        return kGongModeRatios[mode];
    return 1.0f;
}

float Resonator::getEffectiveModeFrequency(int mode) const
{
    if (mode < 0 || mode >= kMaxModes)
        return fundamentalFrequency;

    float baseRatio = kGongModeRatios[mode];
    float userRatio = modeParams[mode].frequencyRatio;
    float detuneRatio = centsToRatio(modeParams[mode].detuneCents);

    return fundamentalFrequency * baseRatio * userRatio * detuneRatio;
}

float Resonator::centsToRatio(float cents) const
{
    // Convert cents to frequency ratio: ratio = 2^(cents/1200)
    return std::pow(2.0f, cents / 1200.0f);
}

void Resonator::updateFilters()
{
    needsUpdate = false;

    for (int mode = 0; mode < kMaxModes; ++mode)
    {
        float baseFreq = getEffectiveModeFrequency(mode);

        // Apply stereo width - detune L and R channels slightly
        // Width of 1.0 = +/- 0.3% frequency shift (~5 cents)
        float widthAmount = modeParams[mode].stereoWidth * 0.003f;
        modeFreqL[mode] = baseFreq * (1.0f - widthAmount);
        modeFreqR[mode] = baseFreq * (1.0f + widthAmount);

        // Skip if frequency is too high
        if (baseFreq > sampleRate * 0.45f)
        {
            modeGains[mode] = 0.0f;
            continue;
        }

        // Calculate Q based on decay time with per-mode multiplier
        // Q = π * f * T60 / ln(1000) ≈ π * f * T60 / 6.91
        float baseDecayMult = kDefaultModeDecayMultipliers[mode];
        float userDecayMult = modeParams[mode].decayMultiplier;
        float modeDecay = decayTime * baseDecayMult * userDecayMult;

        float Q = juce::MathConstants<float>::pi * baseFreq * modeDecay / 6.91f;
        Q = juce::jlimit(1.0f, 500.0f, Q);

        // Create bandpass/resonant coefficients for L and R
        auto coefficientsL = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, modeFreqL[mode], Q);
        auto coefficientsR = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, modeFreqR[mode], Q);

        *filtersL[mode].coefficients = *coefficientsL;
        *filtersR[mode].coefficients = *coefficientsR;

        // Calculate mode gain based on brightness and per-mode gain
        // Higher modes are attenuated more when brightness is low
        float modePosition = static_cast<float>(mode) / static_cast<float>(kMaxModes - 1);
        float brightnessAttenuation = 1.0f - (1.0f - brightness) * modePosition;
        brightnessAttenuation = brightnessAttenuation * brightnessAttenuation; // Quadratic falloff

        // Apply per-mode gain (dB to linear)
        float modeGainLinear = juce::Decibels::decibelsToGain(modeParams[mode].gainDb);

        modeGains[mode] = brightnessAttenuation * modeGainLinear;
    }
}
