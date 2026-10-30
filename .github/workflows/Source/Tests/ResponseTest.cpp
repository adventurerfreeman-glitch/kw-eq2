// Plain C++ test (no JUCE): g++ -std=c++17 -I../Source ResponseTest.cpp -o t && ./t
#include <cstdio>
#include "KwEq.h"
using namespace kw;
static const double fs[] = { 40, 70, 100, 150, 250, 400, 550, 800, 1100, 1800, 2700, 3800, 5600, 8000, 12000 };
static void row (const char* name, const Settings& s, int onlyGroup = -1)
{
    const double sr = 48000;
    Bank b = makeBank (s, sr);
    std::printf ("%-46s", name);
    for (double f : fs) std::printf ("%6.1f", onlyGroup < 0 ? totalDb (b, f, sr) : groupDb (b, onlyGroup, f, sr));
    std::printf ("\n");
}
int main()
{
    std::printf ("%-46s", "Hz:"); for (double f : fs) std::printf ("%6.0f", f); std::printf ("\n");
    Settings s;
    row ("neutral", s);
    std::printf ("-- TILT alone (mids neutral): zero crossing = pivot (400 / 1100) --\n");
    s = Settings(); s.tilt = 1.f;            row ("TILT full CW, DARK", s);
    s.pivot = 1;                             row ("TILT full CW, BRIGHT", s);
    s = Settings(); s.tilt = 0.f;            row ("TILT full CCW, DARK", s);
    s.pivot = 1;                             row ("TILT full CCW, BRIGHT", s);
    std::printf ("-- MIDS / WIDTH --\n");
    s = Settings(); s.mids = 1.f;            row ("MIDS CW, DARK, BROAD (tilt centre)", s);
    s.width = 1;                             row ("MIDS CW, DARK, FOCUSED (tilt centre)", s);
    s = Settings(); s.mids = 1.f; s.pivot = 1; row ("MIDS CW, BRIGHT, BROAD", s);
    s.width = 1;                             row ("MIDS CW, BRIGHT, FOCUSED", s);
    std::printf ("-- LEAN (mids CW, tilt centre) --\n");
    s = Settings(); s.mids = 1.f; s.lean = 0; row ("DARK BROAD  LEAN LOW", s);
    s.lean = 1;                              row ("DARK BROAD  LEAN HIGH", s);
    s.width = 1; s.lean = 0;                 row ("DARK FOCUSED LEAN LOW", s);
    s.lean = 1;                              row ("DARK FOCUSED LEAN HIGH", s);
    s.on[gLean] = false;                     row ("DARK FOCUSED, LEAN bypassed", s);
    std::printf ("-- NEW FEATURES (that feature only) --\n");
    s = Settings(); s.punch = 1.f;           row ("MUD->PUNCH full", s, gPunch);
    s = Settings(); s.blade = 1.f; s.pivot = 1; s.width = 1; row ("BLADE full, BRIGHT + FOCUSED", s, gBlade);
    s.width = 0;                             row ("BLADE full, BRIGHT + BROAD", s, gBlade);
    s.pivot = 0;                             row ("BLADE full, DARK + BROAD", s, gBlade);
    s = Settings(); s.contour = -1.f;        row ("CONTOUR tight", s, gContour);
    s.contour = 1.f;                         row ("CONTOUR warm", s, gContour);
    s = Settings(); s.cut = 1.f;             row ("CUT full", s, gCut);
    s = Settings(); s.push = 1.f; s.pushHz = 100; row ("PUSH full @100 Hz", s, gPush);
    // peaks
    auto peak = [] (const char* n, Settings st) {
        double mx = -99, fm = 0; Bank b = makeBank (st, 48000);
        for (double f = 30; f < 16000; f *= 1.01) { double d = totalDb (b, f, 48000); if (d > mx) { mx = d; fm = f; } }
        std::printf ("%-46s peak %.2f dB at %.0f Hz\n", n, mx, fm); };
    // independence check: changing TILT/PIVOT must not move the MIDS-section curve
    { Settings a; a.mids = 1.f; a.lean = 1; a.width = 1; a.on[gTilt] = false; a.on[gBlade] = false;
      Settings c2 = a; c2.tilt = 1.f; c2.on[gTilt] = true;      // tilt on and moved
      Bank b1 = makeBank (a, 48000), b2 = makeBank (c2, 48000); double d = 0;
      for (double f = 30; f < 16000; f *= 1.05) d = std::max (d, std::abs (groupDb (b1, gMids, f, 48000) - groupDb (b2, gMids, f, 48000)) + std::abs (groupDb (b1, gLean, f, 48000) - groupDb (b2, gLean, f, 48000)));
      std::printf ("TILT change -> max change in MIDS+LEAN curves: %.3g dB (should be 0)\n", d); }
    std::printf ("-- peaks --\n");
    s = Settings(); s.mids = 1.f; s.tilt = 1.f; s.width = 1; s.lean = 1; s.pivot = 1; peak ("MIDS+TILT CW, FOCUSED, HIGH, BRIGHT", s);
    s.blade = 1.f; peak ("... plus BLADE full", s);
    Channel ch; Settings t; for (int g = 0; g < kNumGroups; ++g) t.on[g] = false; Bank bb = makeBank (t, 48000);
    double e = 0; for (int i = 0; i < 4800; ++i) { float x = std::sin (i * 0.05f); e += std::abs (ch.process (bb, x) - x); }
    std::printf ("all-groups-bypassed error: %.3g (should be 0)\n", e);
    return 0;
}
