#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

// Tilt / Lean / Mids EQ section of the K/W-derived boost (provisional model).
// All numbers below are the Rev-17 working values and are meant to be tweaked.
namespace kw
{
struct EqSettings
{
    float tilt   = 0.5f;   // 0..1, 0.5 = centre
    float mids   = 0.5f;   // 0..1, 0.5 = centre
    int   pivot  = 0;      // 0 = DARK (550 Hz), 1 = BRIGHT (1200 Hz)
    int   lean   = 0;      // 0 = LOW long arm, 1 = HIGH long arm
    int   width  = 0;      // 0 = BROAD, 1 = FOCUSED
};

constexpr float kMidsRangeDb   = 5.5f;   // +/- at the extremes of MIDS
constexpr float kTiltRangeDb   = 1.2f;   // +/- general balance at the extremes of TILT
constexpr float kPivotDark     = 550.0f;
constexpr float kPivotBright   = 1200.0f;
constexpr float kQBroad        = 0.55f;
constexpr float kQFocused      = 1.30f;
constexpr float kSkirtQ        = 0.45f;  // wide "long arm"
constexpr float kSkirtShift    = 1.8f;   // skirt centre = pivot x or / this
constexpr float kSkirtAmount   = 0.50f;  // skirt gain as a fraction of the mids gain (at full asymmetry)
constexpr float kBellShare     = 0.40f;  // main bell gives up this much of its gain at full asymmetry, so the peak stays near the Mids range

using Coeffs = juce::dsp::IIR::Coefficients<float>;

struct Bank
{
    Coeffs::Ptr lowShelf, highShelf, bell, skirt;
    float midsDb = 0, balanceDb = 0, asym = 0, pivotHz = 0;
};

inline Bank makeBank (const EqSettings& s, double sr)
{
    Bank b;
    b.pivotHz   = s.pivot ? kPivotBright : kPivotDark;
    b.midsDb    = (s.mids - 0.5f) * 2.0f * kMidsRangeDb;
    b.balanceDb = (s.tilt - 0.5f) * 2.0f * kTiltRangeDb;
    b.asym      = std::abs (s.tilt - 0.5f) * 2.0f;       // 0 at centre, 1 at either end

    const float fMax = (float) (0.45 * sr);
    auto clampF = [fMax] (float f) { return juce::jlimit (20.0f, fMax, f); };
    auto g = [] (float db) { return juce::Decibels::decibelsToGain (db); };

    const float half = 0.5f * b.balanceDb;                 // shelves share the tilt
    b.lowShelf  = Coeffs::makeLowShelf  (sr, clampF (b.pivotHz), 0.5f, g (-half));
    b.highShelf = Coeffs::makeHighShelf (sr, clampF (b.pivotHz), 0.5f, g (+half));

    b.bell = Coeffs::makePeakFilter (sr, clampF (b.pivotHz), s.width ? kQFocused : kQBroad, g (b.midsDb * (1.0f - kBellShare * b.asym)));

    const float skirtHz = s.lean ? b.pivotHz * kSkirtShift : b.pivotHz / kSkirtShift;
    b.skirt = Coeffs::makePeakFilter (sr, clampF (skirtHz), kSkirtQ,
                                      g (b.midsDb * kSkirtAmount * b.asym));
    return b;
}

// Four-biquad chain per channel.
struct Channel
{
    juce::dsp::IIR::Filter<float> lo, hi, bell, skirt;
    void reset() { lo.reset(); hi.reset(); bell.reset(); skirt.reset(); }
    void set (const Bank& b) { lo.coefficients = b.lowShelf; hi.coefficients = b.highShelf;
                               bell.coefficients = b.bell; skirt.coefficients = b.skirt; }
    float process (float x) { return skirt.processSample (bell.processSample (hi.processSample (lo.processSample (x)))); }
};

inline double magnitudeDb (const Bank& b, double f, double sr)
{
    double m = b.lowShelf->getMagnitudeForFrequency (f, sr) * b.highShelf->getMagnitudeForFrequency (f, sr)
             * b.bell->getMagnitudeForFrequency (f, sr)     * b.skirt->getMagnitudeForFrequency (f, sr);
    return 20.0 * std::log10 (m);
}
} // namespace kw
