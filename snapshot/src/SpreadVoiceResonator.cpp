#include "SpreadVoiceResonator.h"

constexpr float SpreadVoiceResonator::kBasePanPositions[];
constexpr float SpreadVoiceResonator::kBaseDetuneOffsets[];

SpreadVoiceResonator::SpreadVoiceResonator()
{
    voiceGains.fill(1.0f);
    voicePans.fill(0.0f);
    voiceFreqs.fill(220.0f);

    smoothedEnergy.reset(48000.0, 0.02);  // 20ms smoothing
}

void SpreadVoiceResonator::prepare(double newSampleRate, int newBlockSize)
{
    sampleRate = newSampleRate;
    blockSize = newBlockSize;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    for (int i = 0; i < kNumVoices; ++i)
    {
        filtersL[i].prepare(spec);
        filtersR[i].prepare(spec);
    }

    smoothedEnergy.reset(sampleRate, 0.02);  // 20ms smoothing

    needsUpdate = true;
    updateFilters();
}

void SpreadVoiceResonator::process(const juce::AudioBuffer<float>& input, juce::AudioBuffer<float>& output)
{
    if (!isEnabled)
        return;

    if (needsUpdate)
        updateFilters();

    auto numSamples = input.getNumSamples();
    auto numChannels = juce::jmin(input.getNumChannels(), output.getNumChannels());

    if (numChannels < 2)
        return;  // Require stereo for panning

    // Update smoothed energy once per block
    smoothedEnergy.setTargetValue(currentEnergy);
    float energy = smoothedEnergy.skip(numSamples);

    // Calculate energy-modulated parameters for this block
    float effectiveBrightness = baseBrightness + energy * brightnessEnergyAmount * (1.0f - baseBrightness);
    effectiveBrightness = juce::jlimit(0.0f, 1.0f, effectiveBrightness);

    float effectiveSpreadLevel = spreadLevel + energy * spreadLevelEnergyAmount * (1.0f - spreadLevel);
    float effectivePanWidth = spreadPanWidth + energy * panWidthEnergyAmount * (1.0f - spreadPanWidth);

    // Output gain boost and master gain
    constexpr float outputGainBoost = 8.0f;
    float masterGainLinear = juce::Decibels::decibelsToGain(gainDb);
    float totalGain = outputGainBoost * masterGainLinear;

    // Process each voice
    for (int voice = 0; voice < kNumVoices; ++voice)
    {
        // Skip if voice frequency is too high
        if (voiceFreqs[voice] > sampleRate * 0.45f)
            continue;

        // Calculate voice gain based on energy-modulated brightness
        float voiceGain;
        if (voice == kCenterVoice)
            voiceGain = effectiveBrightness * totalGain;
        else
            voiceGain = effectiveSpreadLevel * effectiveBrightness * totalGain;

        // Skip spread voices if their gain is negligible
        if (voice > 0 && voiceGain < 0.001f)
            continue;

        // Calculate L/R gains from energy-modulated pan width
        float pan = kBasePanPositions[voice] * effectivePanWidth;
        float panL = std::cos((pan + 1.0f) * 0.5f * juce::MathConstants<float>::halfPi);
        float panR = std::sin((pan + 1.0f) * 0.5f * juce::MathConstants<float>::halfPi);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Get input (mono sum for excitation)
            float inSample = (input.getSample(0, sample) + input.getSample(1, sample)) * 0.5f;

            // Filter through L and R channels
            float filteredL = filtersL[voice].processSample(inSample);
            float filteredR = filtersR[voice].processSample(inSample);

            // Apply voice gain and panning
            output.addSample(0, sample, filteredL * voiceGain * panL);
            output.addSample(1, sample, filteredR * voiceGain * panR);
        }
    }
}

void SpreadVoiceResonator::reset()
{
    for (int i = 0; i < kNumVoices; ++i)
    {
        filtersL[i].reset();
        filtersR[i].reset();
    }
    smoothedEnergy.reset(sampleRate, 0.02);
}

float SpreadVoiceResonator::getEffectiveFrequency() const
{
    if (frequencyMode == FrequencyMode::Snap)
    {
        return static_cast<float>(juce::MidiMessage::getMidiNoteInHertz(midiNote));
    }
    return frequencyHz;
}

void SpreadVoiceResonator::setFrequencyMode(FrequencyMode mode)
{
    if (frequencyMode != mode)
    {
        frequencyMode = mode;
        needsUpdate = true;
    }
}

void SpreadVoiceResonator::setFrequencyHz(float hz)
{
    hz = juce::jlimit(20.0f, 2000.0f, hz);
    if (frequencyHz != hz)
    {
        frequencyHz = hz;
        if (frequencyMode == FrequencyMode::Free)
            needsUpdate = true;
    }
}

void SpreadVoiceResonator::setMidiNote(int note)
{
    note = juce::jlimit(0, 127, note);
    if (midiNote != note)
    {
        midiNote = note;
        if (frequencyMode == FrequencyMode::Snap)
            needsUpdate = true;
    }
}

void SpreadVoiceResonator::setDecayTime(float seconds)
{
    seconds = juce::jlimit(0.1f, 30.0f, seconds);
    if (decayTime != seconds)
    {
        decayTime = seconds;
        needsUpdate = true;
    }
}

void SpreadVoiceResonator::setBaseBrightness(float brightness)
{
    baseBrightness = juce::jlimit(0.0f, 1.0f, brightness);
    needsUpdate = true;
}

void SpreadVoiceResonator::setSpreadLevel(float level)
{
    spreadLevel = juce::jlimit(0.0f, 1.0f, level);
    needsUpdate = true;
}

void SpreadVoiceResonator::setSpreadDetune(float cents)
{
    spreadDetune = juce::jlimit(-100.0f, 100.0f, cents);
    needsUpdate = true;
}

void SpreadVoiceResonator::setSpreadPanWidth(float width)
{
    spreadPanWidth = juce::jlimit(0.0f, 1.0f, width);
    needsUpdate = true;
}

void SpreadVoiceResonator::setGainDb(float db)
{
    gainDb = juce::jlimit(-24.0f, 24.0f, db);
}

void SpreadVoiceResonator::setEnergy(float energy)
{
    currentEnergy = juce::jlimit(0.0f, 1.0f, energy);
    // Don't call updateVoiceParameters here - it causes filter resets every block
    // Voice parameters are updated in process() via smoothedEnergy
}

void SpreadVoiceResonator::setBrightnessEnergyAmount(float amount)
{
    brightnessEnergyAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void SpreadVoiceResonator::setSpreadLevelEnergyAmount(float amount)
{
    spreadLevelEnergyAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void SpreadVoiceResonator::setSpreadDetuneEnergyAmount(float amount)
{
    spreadDetuneEnergyAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void SpreadVoiceResonator::setPanWidthEnergyAmount(float amount)
{
    panWidthEnergyAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void SpreadVoiceResonator::setFrequencyBendEnergyAmount(float amount)
{
    frequencyBendEnergyAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void SpreadVoiceResonator::setPitchGlideDirection(float dir)
{
    glideDirection = juce::jlimit(-1.0f, 1.0f, dir);
}

void SpreadVoiceResonator::setPitchGlideSensitivity(float cents)
{
    glideSensitivity = juce::jlimit(0.0f, 200.0f, cents);
}

void SpreadVoiceResonator::setPitchGlideSmoothing(float coeff)
{
    glideSmoothing = juce::jlimit(0.001f, 0.5f, coeff);
}

void SpreadVoiceResonator::updateVoiceParameters()
{
    float energy = currentEnergy;
    float baseFreq = getEffectiveFrequency();

    // Step 6: Nonlinear pitch glide with smoothing
    float targetBend = energy * glideSensitivity * glideDirection;
    float maxBend = 200.0f;
    targetBend = juce::jlimit(-maxBend, maxBend, targetBend);
    currentBend += (targetBend - currentBend) * glideSmoothing;
    float freqBendRatio = std::pow(2.0f, currentBend / 1200.0f);
    baseFreq *= freqBendRatio;

    // Calculate effective brightness (higher energy = brighter)
    float effectiveBrightness = baseBrightness + energy * brightnessEnergyAmount * (1.0f - baseBrightness);
    effectiveBrightness = juce::jlimit(0.0f, 1.0f, effectiveBrightness);

    // Calculate effective spread level
    float effectiveSpreadLevel = spreadLevel + energy * spreadLevelEnergyAmount * (1.0f - spreadLevel);
    effectiveSpreadLevel = juce::jlimit(0.0f, 1.0f, effectiveSpreadLevel);

    // Calculate effective detune
    float effectiveDetune = spreadDetune * (1.0f + energy * spreadDetuneEnergyAmount);

    // Calculate effective pan width
    float effectivePanWidth = spreadPanWidth + energy * panWidthEnergyAmount * (1.0f - spreadPanWidth);
    effectivePanWidth = juce::jlimit(0.0f, 1.0f, effectivePanWidth);

    // Update per-voice parameters
    for (int voice = 0; voice < kNumVoices; ++voice)
    {
        // Calculate voice frequency with detune
        float detuneOffset = kBaseDetuneOffsets[voice] + (voice > 0 ? effectiveDetune : 0.0f);
        float detuneRatio = std::pow(2.0f, detuneOffset / 1200.0f);
        voiceFreqs[voice] = baseFreq * detuneRatio;

        // Calculate voice pan with width scaling
        voicePans[voice] = kBasePanPositions[voice] * effectivePanWidth;

        // Calculate voice gain
        if (voice == kCenterVoice)
        {
            // Center voice always at full level, modulated by brightness
            voiceGains[voice] = effectiveBrightness;
        }
        else
        {
            // Spread voices scaled by spread level and brightness
            voiceGains[voice] = effectiveSpreadLevel * effectiveBrightness;
        }
    }

    needsUpdate = true;
}

void SpreadVoiceResonator::updateFilters()
{
    needsUpdate = false;

    // Make sure voice parameters are current
    updateVoiceParameters();

    float energy = currentEnergy;

    // Calculate effective brightness for Q modulation
    float effectiveBrightness = baseBrightness + energy * brightnessEnergyAmount * (1.0f - baseBrightness);
    effectiveBrightness = juce::jlimit(0.0f, 1.0f, effectiveBrightness);

    for (int voice = 0; voice < kNumVoices; ++voice)
    {
        float freq = voiceFreqs[voice];

        // Skip if frequency is too high
        if (freq > sampleRate * 0.45f)
            continue;

        // Calculate Q based on decay time
        // The theoretical Q = π * f * T60 / ln(1000) gives very high Q values
        // which makes filters too narrow for audio input. Use a more practical formula.
        // Base Q in range 5-50, modulated by decay time and brightness
        float baseQ = 5.0f + (decayTime / 15.0f) * 25.0f;  // 5-30 based on decay (0.5-15s)

        // Modulate Q by brightness (higher brightness = higher Q, more resonant)
        float brightnessQMod = 0.5f + effectiveBrightness * 1.5f;  // Range 0.5 to 2.0
        float Q = baseQ * brightnessQMod;
        Q = juce::jlimit(2.0f, 80.0f, Q);  // Keep Q reasonable for audio input

        // Create bandpass coefficients and assign (don't dereference - coefficients might be null)
        auto coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(sampleRate, freq, Q);

        filtersL[voice].coefficients = coefficients;
        filtersR[voice].coefficients = coefficients;
        // Don't reset filter state here - it causes audio glitches
        // Filters are only reset in prepare() and reset()
    }
}
