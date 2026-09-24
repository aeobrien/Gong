#pragma once

#include <JuceHeader.h>
#include "ModulationTypes.h"
#include <atomic>
#include <array>
#include <cmath>

struct StageMeter {
    std::atomic<float> rmsL{0}, rmsR{0}, peakL{0}, peakR{0};

    void update(const juce::AudioBuffer<float>& buf, int start, int numSamples)
    {
        if (numSamples <= 0) return;

        float sumL = 0, sumR = 0;
        float pkL = 0, pkR = 0;

        auto* chL = buf.getReadPointer(0);
        for (int i = start; i < start + numSamples; ++i)
        {
            float s = chL[i];
            sumL += s * s;
            float a = std::fabs(s);
            if (a > pkL) pkL = a;
        }

        if (buf.getNumChannels() > 1)
        {
            auto* chR = buf.getReadPointer(1);
            for (int i = start; i < start + numSamples; ++i)
            {
                float s = chR[i];
                sumR += s * s;
                float a = std::fabs(s);
                if (a > pkR) pkR = a;
            }
        }
        else
        {
            sumR = sumL;
            pkR = pkL;
        }

        rmsL.store(std::sqrt(sumL / numSamples), std::memory_order_relaxed);
        rmsR.store(std::sqrt(sumR / numSamples), std::memory_order_relaxed);
        peakL.store(pkL, std::memory_order_relaxed);
        peakR.store(pkR, std::memory_order_relaxed);
    }
};

struct StageTiming {
    std::atomic<float> lastUs{0}, maxUs{0};

    void record(float us)
    {
        lastUs.store(us, std::memory_order_relaxed);
        float prev = maxUs.load(std::memory_order_relaxed);
        if (us > prev)
            maxUs.store(us, std::memory_order_relaxed);
    }

    void resetMax() { maxUs.store(0, std::memory_order_relaxed); }
};

struct DiagnosticState {
    // Meters (6 points in chain)
    StageMeter input, afterSynth, afterConv, afterExciter, afterComp, output;

    // Energy (read from EnergyAccumulator)
    std::atomic<float> globalEnergy{0};
    std::array<std::atomic<float>, 4> bandEnergy{};

    // Timing per stage
    StageTiming synthTime, convTime, exciterTime, compTime;
    std::atomic<float> totalTimeUs{0}, budgetUs{0};

    // Solo/Mute/Bypass: 0=normal, 1=mute, 2=solo, 3=bypass
    std::atomic<int> synthState{0}, convState{0}, exciterState{0}, compState{0};

    // Test signal config
    std::atomic<int> testSignalType{0};      // 0=off,1=sine,2=sweep,3=white,4=pink,5=impulse
    std::atomic<int> testInjectionPoint{0};  // 0=input,1=afterSynth,2=afterConv,3=afterExciter
    std::atomic<float> testSignalGain{0.5f};
    std::atomic<float> testSineFreq{440.0f};

    // Modulation routes: double-buffered (max 32)
    static constexpr int kMaxRoutes = 32;
    std::array<ModRoute, kMaxRoutes> routesA{}, routesB{};
    std::atomic<int> routeCountA{0}, routeCountB{0};
    std::atomic<int> activeBuffer{0};  // 0=A, 1=B

    // LFO config
    std::atomic<float> lfo1Rate{0.5f}, lfo2Rate{2.0f};
    std::atomic<int> lfo1Shape{0}, lfo2Shape{1};  // 0=sine,1=tri,2=saw,3=square

    // Helper: get active routes for audio thread
    const ModRoute* getActiveRoutes(int& count) const
    {
        if (activeBuffer.load(std::memory_order_acquire) == 0)
        {
            count = routeCountA.load(std::memory_order_relaxed);
            return routesA.data();
        }
        else
        {
            count = routeCountB.load(std::memory_order_relaxed);
            return routesB.data();
        }
    }

    // Helper: write routes from UI thread to inactive buffer, then swap
    void setRoutes(const ModRoute* routes, int count)
    {
        count = juce::jmin(count, kMaxRoutes);
        int inactive = activeBuffer.load(std::memory_order_acquire) == 0 ? 1 : 0;

        if (inactive == 0)
        {
            for (int i = 0; i < count; ++i) routesA[i] = routes[i];
            routeCountA.store(count, std::memory_order_relaxed);
        }
        else
        {
            for (int i = 0; i < count; ++i) routesB[i] = routes[i];
            routeCountB.store(count, std::memory_order_relaxed);
        }

        activeBuffer.store(inactive, std::memory_order_release);
    }

    // Check if any stage is soloed
    bool isAnySoloed() const
    {
        return synthState.load(std::memory_order_relaxed) == 2 ||
               convState.load(std::memory_order_relaxed) == 2 ||
               exciterState.load(std::memory_order_relaxed) == 2 ||
               compState.load(std::memory_order_relaxed) == 2;
    }

    // For a given stage state and solo context: should the stage process?
    // Returns: 0=process normally, 1=mute (zero after), 2=bypass (skip)
    int getEffectiveState(int stageState, bool anySoloed) const
    {
        if (stageState == 3) return 2; // bypass
        if (stageState == 1) return 1; // mute
        if (anySoloed && stageState != 2) return 2; // not soloed while others are
        return 0; // normal
    }

    void resetTimingMaxes()
    {
        synthTime.resetMax();
        convTime.resetMax();
        exciterTime.resetMax();
        compTime.resetMax();
    }
};
