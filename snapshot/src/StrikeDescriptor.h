#pragma once

#include <cstdint>
#include <cmath>

/**
 * Plain data struct matching the Teensy's 11-byte strike packet payload.
 * Describes the physical characteristics of a single strike event.
 */
struct StrikeDescriptor
{
    uint8_t padId = 0;              // 0-3 → resonator index
    uint8_t velocity = 0;           // 0-127
    uint8_t attackSlope = 128;      // 0=soft 3ms ramp, 255=hard 1-sample click
    uint16_t spectralCentroid = 2000; // Hz, controls noise filter cutoff
    uint8_t hfEnergyRatio = 128;    // 0=fully filtered, 255=white noise
    uint8_t decayShape = 128;       // 0=short 0.5ms, 255=long 5ms decay

    // Normalized accessors (0.0 to 1.0)
    float velocityNorm() const      { return velocity / 127.0f; }
    float attackSlopeNorm() const   { return attackSlope / 255.0f; }
    float hfEnergyRatioNorm() const { return hfEnergyRatio / 255.0f; }
    float decayShapeNorm() const    { return decayShape / 255.0f; }

    // Spectral centroid mapped to filter cutoff range (200-15000 Hz)
    float spectralCentroidHz() const
    {
        return static_cast<float>(spectralCentroid);
    }
};
