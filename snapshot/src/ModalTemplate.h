#pragma once

#include <array>

/**
 * Step 11: Modal frequency templates from McLachlan's measurements.
 * Each template defines frequency ratios relative to fundamental for
 * different gong types. Used by ResonatorBank to set physically-accurate
 * modal frequencies.
 */
struct ModalTemplate {
    const char* name;
    std::array<float, 28> ratios;  // Frequency ratios relative to fundamental
};

// Chinese opera gong (chau gong) - dense, relatively harmonic partials
static const ModalTemplate kChineseGong = {
    "Chinese Gong",
    { 1.0f, 1.52f, 2.08f, 2.56f, 3.12f, 3.68f, 4.16f, 4.72f,
      5.20f, 5.84f, 6.32f, 6.96f, 7.44f, 8.08f, 8.64f, 9.28f,
      9.84f, 10.48f, 11.04f, 11.68f, 12.24f, 12.88f, 13.44f, 14.08f,
      14.64f, 15.28f, 15.84f, 16.48f }
};

// Large tam-tam - inharmonic, closely spaced low modes
static const ModalTemplate kTamTam = {
    "Tam-Tam",
    { 1.0f, 1.31f, 1.67f, 2.09f, 2.44f, 2.91f, 3.35f, 3.86f,
      4.28f, 4.82f, 5.31f, 5.89f, 6.37f, 6.98f, 7.52f, 8.16f,
      8.73f, 9.41f, 10.02f, 10.74f, 11.39f, 12.15f, 12.84f, 13.64f,
      14.37f, 15.21f, 15.98f, 16.86f }
};

// Gamelan gong (bonang/kenong) - wide spacing, emphasis on octave relationships
static const ModalTemplate kGamelan = {
    "Gamelan",
    { 1.0f, 2.76f, 5.40f, 8.93f, 13.34f, 1.55f, 3.92f, 7.06f,
      11.08f, 15.98f, 2.14f, 4.67f, 8.08f, 12.37f, 17.54f, 1.83f,
      3.38f, 5.81f, 9.12f, 13.31f, 18.38f, 2.47f, 4.23f, 6.87f,
      10.42f, 14.85f, 20.16f, 3.05f }
};

// Tibetan singing bowl - near-harmonic with characteristic beating
static const ModalTemplate kTibetanBowl = {
    "Tibetan Bowl",
    { 1.0f, 2.71f, 5.00f, 7.73f, 10.95f, 14.65f, 18.84f, 1.48f,
      3.89f, 6.88f, 10.35f, 14.30f, 18.73f, 2.09f, 4.58f, 7.65f,
      11.20f, 15.23f, 19.74f, 2.38f, 5.14f, 8.48f, 12.30f, 16.60f,
      21.38f, 1.76f, 3.29f, 5.92f }
};

// Flat gong (nipple-less) - maximally inharmonic
static const ModalTemplate kFlatGong = {
    "Flat Gong",
    { 1.0f, 1.47f, 2.01f, 2.63f, 3.31f, 4.07f, 4.89f, 5.78f,
      6.73f, 7.75f, 8.83f, 9.98f, 11.19f, 12.47f, 13.81f, 15.22f,
      16.69f, 18.23f, 19.83f, 21.50f, 23.23f, 25.03f, 26.89f, 28.82f,
      30.81f, 32.87f, 34.99f, 37.18f }
};

static const ModalTemplate* const kAllTemplates[] = {
    &kChineseGong,
    &kTamTam,
    &kGamelan,
    &kTibetanBowl,
    &kFlatGong
};

static constexpr int kNumTemplates = 5;
