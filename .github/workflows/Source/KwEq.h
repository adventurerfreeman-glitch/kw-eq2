#pragma once
// KW Tilt / Lean EQ  -  DSP core (plain C++, no JUCE dependency, so it can be unit-tested).
// Version 0.2.  Every tweakable number is a named constant in namespace kw::cfg.
#include <algorithm>
#include <array>
#include <cmath>

namespace kw
{
namespace cfg
{
// ---- TILT: bounded ramp between a low shoulder and a high shoulder, zero at the pivot ----
constexpr float tiltDb          = 1.44f;   // dB at each shoulder at full TILT (v0.1 was 1.2; x1.2 = +20%)
constexpr float shoulderLowHz   = 100.0f;  // below this the tilt stops (flat) -> joins the circuit's own rolloff
constexpr float shoulderHighHz  = 5600.0f; // above this the tilt stops (flat)
constexpr float pivotDarkHz     = 550.0f;
constexpr float pivotBrightHz   = 1200.0f;
// Two broad shelves fitted (numerically, <0.1 dB RMS) to that ramp, per pivot:
constexpr float tiltLoHzDark    = 261.5f, tiltHiHzDark   = 1628.0f;
constexpr float tiltLoHzBright  = 394.8f, tiltHiHzBright = 2324.0f;
constexpr float tiltQ           = 0.5f;

// ---- MIDS ----
constexpr float midsRangeDb     = 5.5f;    // +/- at the extremes of MIDS
constexpr float qBroad          = 0.35f;   // very broad
constexpr float qFocused        = 2.0f;    // clearly tighter than broad
constexpr float focusedGain     = 1.25f;   // focused bell gets a little extra gain so it does not feel quieter

// ---- LEAN (asymmetry) ----
constexpr float leanShift       = 2.2f;    // long-arm skirt centre = pivot x or / this
constexpr float leanSkirtQ      = 0.40f;
constexpr float leanSkirtAmount = 0.85f;   // skirt gain, fraction of the mids gain
constexpr float leanBellShare   = 0.45f;   // main bell gives up this much at full asymmetry (keeps the peak sane)
constexpr float leanShortShift  = 1.7f;    // short-arm dip centre = pivot / or x this
constexpr float leanShortQ      = 1.2f;
constexpr float leanShortAmount = 0.35f;   // short-arm dip depth, fraction of the mids gain (opposite sign)

// ---- LOW CONTOUR (tight/punchy <-> smooth/warm) ----
constexpr float contourHpfTightHz = 115.0f; // HPF corner at full TIGHT
constexpr float contourPunchHz    = 150.0f;
constexpr float contourPunchDb    = 2.5f;
constexpr float contourPunchQ     = 1.1f;
constexpr float contourWarmHz     = 130.0f;
constexpr float contourWarmDb     = 3.5f;
constexpr float contourDipHz      = 320.0f; // smooths the box when going warm
constexpr float contourDipDb      = -1.5f;
constexpr float contourDipQ       = 0.8f;

// ---- OUTPUT: CUT (vox-style top cut + presence) and PUSH (low resonance) ----
constexpr float cutLpMaxHz   = 14000.0f;   // 1-pole low-pass corner at CUT = 0
constexpr float cutLpMinHz   = 3200.0f;    // ... and at CUT = 1
constexpr float cutPresHz    = 3800.0f;
constexpr float cutPresDb    = -6.0f;      // presence shelf cut at CUT = 1
constexpr float pushMaxDb    = 6.0f;
constexpr float pushQ        = 1.6f;
}

enum Group { gTilt = 0, gMids, gLean, gContour, gCut, gPush, kNumGroups };
constexpr int kNumFilters = 12;
inline const char* groupName (int g)
{ static const char* n[] = { "Tilt", "Mids", "Lean", "Contour", "Cut", "Push" }; return n[g]; }

struct Settings
{
    float tilt = 0.5f, mids = 0.5f;         // 0..1, 0.5 = centre
    int   pivot = 0, lean = 0, width = 0;   // toggles
    float leanDepth = 1.0f;                 // multiplier on asymmetry strength (0..1.5)
    float contour = 0.0f;                   // -1 (tight) .. +1 (warm)
    float cut = 0.0f, push = 0.0f, pushHz = 100.0f;   // 0..1, 0..1, Hz
    bool  on[kNumGroups] = { true, true, true, true, true, true };   // per-feature enable (false = bypassed)
    float volumeDb = 0.0f;
};

struct Coef { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };   // normalised (a0 = 1)

namespace rbj
{
inline Coef norm (double b0, double b1, double b2, double a0, double a1, double a2)
{ Coef c; c.b0 = (float) (b0 / a0); c.b1 = (float) (b1 / a0); c.b2 = (float) (b2 / a0);
  c.a1 = (float) (a1 / a0); c.a2 = (float) (a2 / a0); return c; }

inline double lim (double f, double sr) { return std::min (std::max (f, 15.0), 0.45 * sr); }

inline Coef peak (double sr, double f, double Q, double dB)
{ const double A = std::pow (10.0, dB / 40), w = 2 * M_PI * lim (f, sr) / sr, c = std::cos (w), al = std::sin (w) / (2 * Q);
  return norm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A); }

inline Coef lowShelf (double sr, double f, double Q, double dB)
{ const double A = std::pow (10.0, dB / 40), w = 2 * M_PI * lim (f, sr) / sr, c = std::cos (w), s = 2 * std::sqrt (A) * std::sin (w) / (2 * Q);
  return norm (A * ((A + 1) - (A - 1) * c + s), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - s),
               (A + 1) + (A - 1) * c + s, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - s); }

inline Coef highShelf (double sr, double f, double Q, double dB)
{ const double A = std::pow (10.0, dB / 40), w = 2 * M_PI * lim (f, sr) / sr, c = std::cos (w), s = 2 * std::sqrt (A) * std::sin (w) / (2 * Q);
  return norm (A * ((A + 1) + (A - 1) * c + s), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - s),
               (A + 1) - (A - 1) * c + s, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - s); }

inline Coef highPass (double sr, double f, double Q)
{ const double w = 2 * M_PI * lim (f, sr) / sr, c = std::cos (w), al = std::sin (w) / (2 * Q);
  return norm ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al); }

inline Coef lowPass1 (double sr, double f)
{ const double K = std::tan (M_PI * lim (f, sr) / sr);
  return norm (K, K, 0, 1 + K, K - 1, 0); }
}

struct Bank
{
    std::array<Coef, kNumFilters> c;
    std::array<int,  kNumFilters> group {};
    std::array<bool, kNumFilters> active {};   // false = identity (skipped)
    float outGain = 1.0f;
    // handy for the display / tests
    float midsDb = 0, asym = 0, pivotHz = 0;
};

inline Bank makeBank (const Settings& s, double sr)
{
    using namespace cfg;
    Bank b;
    for (int i = 0; i < kNumFilters; ++i) b.active[i] = false;
    auto set = [&b] (int i, int g, const Coef& c, bool on) { b.c[i] = c; b.group[i] = g; b.active[i] = on; };

    const bool dark = (s.pivot == 0);
    const double piv = dark ? pivotDarkHz : pivotBrightHz;
    b.pivotHz = (float) piv;

    // 0,1: TILT  (positive = brighter: lows down, highs up, ramp between the two shoulders)
    const double T = (s.tilt - 0.5) * 2.0 * tiltDb;
    set (0, gTilt, rbj::lowShelf  (sr, dark ? tiltLoHzDark : tiltLoHzBright, tiltQ, -T), s.on[gTilt] && std::abs (T) > 1e-4);
    set (1, gTilt, rbj::highShelf (sr, dark ? tiltHiHzDark : tiltHiHzBright, tiltQ, +T), s.on[gTilt] && std::abs (T) > 1e-4);

    // 2: MIDS main bell, 3: LEAN long-arm skirt, 4: LEAN short-arm dip
    b.midsDb = (s.mids - 0.5f) * 2.0f * midsRangeDb;
    b.asym   = std::min (1.0f, std::abs (s.tilt - 0.5f) * 2.0f) * s.leanDepth;
    const float asymC = std::min (1.0f, b.asym);
    const bool  leanOn = s.on[gLean];
    const float widthGain = s.width ? focusedGain : 1.0f;
    const float bellDb = b.midsDb * widthGain * (leanOn ? (1.0f - leanBellShare * asymC) : 1.0f);
    set (2, gMids, rbj::peak (sr, piv, s.width ? qFocused : qBroad, bellDb), s.on[gMids] && std::abs (bellDb) > 1e-4f);

    const double skirtHz = s.lean ? piv * leanShift : piv / leanShift;
    const double shortHz = s.lean ? piv / leanShortShift : piv * leanShortShift;
    const float skirtDb = b.midsDb * leanSkirtAmount * b.asym;
    const float shortDb = -b.midsDb * leanShortAmount * b.asym;
    set (3, gLean, rbj::peak (sr, skirtHz, leanSkirtQ, skirtDb), leanOn && std::abs (skirtDb) > 1e-4f);
    set (4, gLean, rbj::peak (sr, shortHz, leanShortQ, shortDb), leanOn && std::abs (shortDb) > 1e-4f);

    // 5..8: LOW CONTOUR   (-1 tight/punchy ... 0 off ... +1 warm/large)
    const float tight = std::max (0.0f, -s.contour), warm = std::max (0.0f, s.contour);
    const bool  cOn = s.on[gContour];
    set (5, gContour, rbj::highPass (sr, 20.0 + (contourHpfTightHz - 20.0) * tight, 0.707), cOn && tight > 1e-3f);
    set (6, gContour, rbj::peak (sr, contourPunchHz, contourPunchQ, contourPunchDb * tight), cOn && tight > 1e-3f);
    set (7, gContour, rbj::lowShelf (sr, contourWarmHz, 0.6, contourWarmDb * warm), cOn && warm > 1e-3f);
    set (8, gContour, rbj::peak (sr, contourDipHz, contourDipQ, contourDipDb * warm), cOn && warm > 1e-3f);

    // 9,10: CUT  (vox-style: 1-pole low-pass sweeping down + a presence shelf cut)
    const double lp = cutLpMaxHz * std::pow ((double) cutLpMinHz / cutLpMaxHz, (double) s.cut);
    set (9,  gCut, rbj::lowPass1 (sr, lp), s.on[gCut] && s.cut > 1e-3f);
    set (10, gCut, rbj::highShelf (sr, cutPresHz, 0.6, cutPresDb * s.cut), s.on[gCut] && s.cut > 1e-3f);

    // 11: PUSH  (resonant low peak, frequency selectable)
    set (11, gPush, rbj::peak (sr, s.pushHz, pushQ, pushMaxDb * s.push), s.on[gPush] && s.push > 1e-3f);

    b.outGain = std::pow (10.0f, s.volumeDb / 20.0f);
    return b;
}

// Transposed direct-form II biquad, no allocation.
struct Biquad
{
    float z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    float process (const Coef& c, float x)
    { const float y = c.b0 * x + z1; z1 = c.b1 * x - c.a1 * y + z2; z2 = c.b2 * x - c.a2 * y; return y; }
};

struct Channel
{
    std::array<Biquad, kNumFilters> f;
    void reset() { for (auto& b : f) b.reset(); }
    float process (const Bank& b, float x)
    {
        for (int i = 0; i < kNumFilters; ++i)
            if (b.active[i]) x = f[(size_t) i].process (b.c[(size_t) i], x);
        return x * b.outGain;
    }
};

// ---- analysis helpers (display + tests) ----
inline double magnitudeDb (const Coef& c, double fHz, double sr)
{
    const double w = 2 * M_PI * fHz / sr, c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
    const double nr = c.b0 + c.b1 * c1 + c.b2 * c2, ni = -(c.b1 * s1 + c.b2 * s2);
    const double dr = 1 + c.a1 * c1 + c.a2 * c2,    di = -(c.a1 * s1 + c.a2 * s2);
    return 10.0 * std::log10 ((nr * nr + ni * ni) / (dr * dr + di * di) + 1e-30);
}
inline double groupDb (const Bank& b, int g, double fHz, double sr)
{ double d = 0; for (int i = 0; i < kNumFilters; ++i) if (b.active[i] && b.group[i] == g) d += magnitudeDb (b.c[i], fHz, sr); return d; }
inline double totalDb (const Bank& b, double fHz, double sr)
{ double d = 0; for (int i = 0; i < kNumFilters; ++i) if (b.active[i]) d += magnitudeDb (b.c[i], fHz, sr); return d; }
} // namespace kw
