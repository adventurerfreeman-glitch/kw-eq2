#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

// KW Tilt / Lean EQ - Version 0.2
// Stronger lean, stronger tilt, Dark 400 Hz / Bright 2 kHz

namespace kw
{
struct EqSettings
{
    float tilt   = 0.5f;
    float mids   = 0.5f;
    int   pivot  = 0;      // 0 = Dark 400 Hz, 1 = Bright 2 kHz
    int   lean   = 0;      // 0 = Low, 1 = High
    int   width  = 0;      // 0 = Broad, 1 = Focused

    bool  bypassTilt  = false;
    bool  bypassMids  = false;
    bool  bypassLean  = false;
};

// ===================== TUNABLE CONSTANTS =====================
constexpr float kMidsRangeDb     = 5.5f;
constexpr float kTiltRangeDb     = 1.45f;   // ~20% stronger

constexpr float kPivotDark       = 400.0f;
constexpr float kPivotBright     = 2000.0f;

constexpr float kQBroad          = 0.42f;
constexpr float kQFocused        = 1.90f;

constexpr float kSkirtQ          = 0.42f;
constexpr float kSkirtShift      = 2.15f;
constexpr float kSkirtAmount     = 0.78f;
constexpr float kBellShare       = 0.28f;

constexpr float kLowShoulder     = 100.0f;
constexpr float kHighShoulder    = 5500.0f;
// =============================================================

using Coeffs = juce::dsp::IIR::Coefficients<float>;

struct Bank
{
    Coeffs::Ptr lowShelf, highShelf, bell, skirt;
    float midsDb = 0.0f;
    float balanceDb = 0.0f;
    float asym = 0.0f;
    float pivotHz = 0.0f;
};

inline Bank makeBank (const EqSettings& s, double sr)
{
    Bank b;
    b.pivotHz   = s.pivot ? kPivotBright : kPivotDark;
    b.midsDb    = (s.mids - 0.5f) * 2.0f * kMidsRangeDb;
    b.balanceDb = (s.tilt - 0.5f) * 2.0f * kTiltRangeDb;
    b.asym      = std::abs (s.tilt - 0.5f) * 2.0f;

    const float fMax = (float) (0.45 * sr);
    auto clampF = [fMax] (float f) { return juce::jlimit (20.0f, fMax, f); };
    auto g = [] (float db) { return juce::Decibels::decibelsToGain (db); };

    // Tilt shelves
    float half = 0.5f * b.balanceDb;
    if (s.bypassTilt)
    {
        b.lowShelf  = Coeffs::makeLowShelf  (sr, clampF (b.pivotHz), 0.5f, 1.0f);
        b.highShelf = Coeffs::makeHighShelf (sr, clampF (b.pivotHz), 0.5f, 1.0f);
    }
    else
    {
        b.lowShelf  = Coeffs::makeLowShelf  (sr, clampF (b.pivotHz), 0.5f, g (-half));
        b.highShelf = Coeffs::makeHighShelf (sr, clampF (b.pivotHz), 0.5f, g (+half));
    }

    // Main Mids bell
    float bellGain = s.bypassMids ? 0.0f : b.midsDb * (1.0f - kBellShare * b.asym);
    float q = s.width ? kQFocused : kQBroad;
    b.bell = Coeffs::makePeakFilter (sr, clampF (b.pivotHz), q, g (bellGain));

    // Lean skirt
    float skirtGain = 0.0f;
    if (!s.bypassLean && !s.bypassMids)
        skirtGain = b.midsDb * kSkirtAmount * b.asym;

    float skirtHz = s.lean ? b.pivotHz * kSkirtShift : b.pivotHz / kSkirtShift;
    b.skirt = Coeffs::makePeakFilter (sr, clampF (skirtHz), kSkirtQ, g (skirtGain));

    return b;
}

struct Channel
{
    juce::dsp::IIR::Filter<float> lo, hi, bell, skirt;

    void reset() { lo.reset(); hi.reset(); bell.reset(); skirt.reset(); }

    void set (const Bank& b)
    {
        lo.coefficients    = b.lowShelf;
        hi.coefficients    = b.highShelf;
        bell.coefficients  = b.bell;
        skirt.coefficients = b.skirt;
    }

    float process (float x)
    {
        return skirt.processSample (bell.processSample (hi.processSample (lo.processSample (x))));
    }
};

inline double magnitudeDb (const Bank& b, double f, double sr)
{
    double m = b.lowShelf->getMagnitudeForFrequency (f, sr)
             * b.highShelf->getMagnitudeForFrequency (f, sr)
             * b.bell->getMagnitudeForFrequency (f, sr)
             * b.skirt->getMagnitudeForFrequency (f, sr);
    return 20.0 * std::log10 (m + 1.0e-12);
}

} // namespace kw
