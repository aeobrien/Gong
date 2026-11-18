#include "TestSignalGenerator.h"
#include <cmath>

void TestSignalGenerator::prepare(double sr)
{
    sampleRate = sr;
    phase = 0.0;
    sweepPhase = 0.0;
    sweepSampleCount = 0;
    impulseSampleCount = 0;
    std::memset(pinkState, 0, sizeof(pinkState));
}

void TestSignalGenerator::generateBlock(juce::AudioBuffer<float>& dest, int numSamples,
                                         int signalType, float gain, float sineFreq)
{
    if (signalType <= 0 || numSamples <= 0)
        return;

    auto* outL = dest.getWritePointer(0);
    auto* outR = dest.getNumChannels() > 1 ? dest.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = 0.0f;

        switch (signalType)
        {
            case 1: // Sine
            {
                sample = static_cast<float>(std::sin(phase * 2.0 * juce::MathConstants<double>::pi));
                phase += sineFreq / sampleRate;
                if (phase >= 1.0) phase -= 1.0;
                break;
            }
            case 2: // Sine sweep 20Hz->20kHz over 5s (log)
            {
                double sweepDuration = 5.0 * sampleRate;
                double t = sweepSampleCount / sweepDuration;
                double freq = 20.0 * std::pow(1000.0, t); // 20 to 20000
                sample = static_cast<float>(std::sin(sweepPhase * 2.0 * juce::MathConstants<double>::pi));
                sweepPhase += freq / sampleRate;
                if (sweepPhase >= 1.0) sweepPhase -= std::floor(sweepPhase);
                sweepSampleCount++;
                if (sweepSampleCount >= static_cast<int>(sweepDuration))
                {
                    sweepSampleCount = 0;
                    sweepPhase = 0.0;
                }
                break;
            }
            case 3: // White noise
            {
                sample = random.nextFloat() * 2.0f - 1.0f;
                break;
            }
            case 4: // Pink noise (Paul Kellet's method)
            {
                float white = random.nextFloat() * 2.0f - 1.0f;
                pinkState[0] = 0.99886f * pinkState[0] + white * 0.0555179f;
                pinkState[1] = 0.99332f * pinkState[1] + white * 0.0750759f;
                pinkState[2] = 0.96900f * pinkState[2] + white * 0.1538520f;
                pinkState[3] = 0.86650f * pinkState[3] + white * 0.3104856f;
                pinkState[4] = 0.55000f * pinkState[4] + white * 0.5329522f;
                pinkState[5] = -0.7616f * pinkState[5] - white * 0.0168980f;
                sample = (pinkState[0] + pinkState[1] + pinkState[2] + pinkState[3]
                         + pinkState[4] + pinkState[5] + pinkState[6] + white * 0.5362f) * 0.11f;
                pinkState[6] = white * 0.115926f;
                break;
            }
            case 5: // Impulse every 0.5s
            {
                int period = static_cast<int>(sampleRate * 0.5);
                sample = (impulseSampleCount == 0) ? 1.0f : 0.0f;
                impulseSampleCount++;
                if (impulseSampleCount >= period) impulseSampleCount = 0;
                break;
            }
            default:
                break;
        }

        sample *= gain;
        outL[i] += sample;
        if (outR) outR[i] += sample;
    }
}
