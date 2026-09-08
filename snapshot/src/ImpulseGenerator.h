#pragma once

#include <JuceHeader.h>

class ImpulseGenerator
{
public:
    enum class ImpulseType
    {
        SingleSample,  // Dirac delta impulse
        NoiseBurst     // Short noise burst
    };

    enum class ExcitationSource
    {
        InternalNoise, // Generate impulse internally (MIDI triggered)
        AudioInput     // Use external audio input as excitation
    };

    enum class FilterType
    {
        Lowpass,
        Highpass,
        Bandpass
    };

    ImpulseGenerator();
    ~ImpulseGenerator() = default;

    void prepare(double sampleRate, int blockSize);
    void trigger(float velocity);
    void process(juce::AudioBuffer<float>& buffer);
    void reset();

    // Excitation source
    void setExcitationSource(ExcitationSource source) { excitationSource = source; }
    ExcitationSource getExcitationSource() const { return excitationSource; }

    // Process with external audio input (when source is AudioInput)
    void processWithAudioInput(juce::AudioBuffer<float>& output,
                                const juce::AudioBuffer<float>& audioInput,
                                float inputGain = 1.0f);

    // Impulse type (for internal source)
    void setImpulseType(ImpulseType type) { impulseType = type; }
    ImpulseType getImpulseType() const { return impulseType; }

    void setNoiseBurstLength(int samples) { noiseBurstLengthSamples = samples; }
    int getNoiseBurstLength() const { return noiseBurstLengthSamples; }

    bool isActive() const { return samplesRemaining > 0; }

    // Filter controls
    void setFilterEnabled(bool enabled);
    bool getFilterEnabled() const { return filterEnabled; }

    void setFilterCutoff(float frequencyHz);
    float getFilterCutoff() const { return filterCutoff; }

    void setFilterResonance(float resonance);
    float getFilterResonance() const { return filterResonance; }

    void setFilterType(FilterType type);
    FilterType getFilterType() const { return filterType; }

    // Velocity curve: 0.5 = sqrt (soft), 1.0 = linear, 2.0 = squared (hard)
    void setVelocityCurve(float curve);
    float getVelocityCurve() const { return velocityCurve; }

    // Map velocity according to curve
    float mapVelocity(float velocity) const;

private:
    void updateFilter();

    ExcitationSource excitationSource = ExcitationSource::InternalNoise;
    ImpulseType impulseType = ImpulseType::NoiseBurst;

    double sampleRate = 48000.0;
    int blockSize = 256;

    std::atomic<bool> triggerPending { false };
    std::atomic<float> pendingVelocity { 0.0f };

    int samplesRemaining = 0;
    int noiseBurstLengthSamples = 64; // ~1.3ms at 48kHz
    float currentVelocity = 0.0f;

    juce::Random random;

    // Filter
    bool filterEnabled = true;
    float filterCutoff = 5000.0f;
    float filterResonance = 0.707f; // Butterworth Q
    FilterType filterType = FilterType::Lowpass;

    juce::dsp::StateVariableTPTFilter<float> filterL;
    juce::dsp::StateVariableTPTFilter<float> filterR;
    bool filterNeedsUpdate = true;

    // Velocity mapping
    float velocityCurve = 1.0f; // Linear by default

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImpulseGenerator)
};
