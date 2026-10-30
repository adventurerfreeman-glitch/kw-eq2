#include <juce_core/juce_core.h>
#include <cstdio>
#include "KwEq.h"
int main()
{
    const double sr = 48000.0;
    const double fs[] = { 50, 100, 200, 300, 400, 550, 700, 900, 1200, 1600, 2400, 3500, 5000, 8000, 12000 };
    struct Case { const char* name; kw::EqSettings s; };
    auto mk = [] (float t, float m, int p, int l, int w) { kw::EqSettings s; s.tilt=t; s.mids=m; s.pivot=p; s.lean=l; s.width=w; return s; };
    Case cases[] = {
        { "neutral (tilt .5, mids .5)",              mk (.5f, .5f, 0, 0, 0) },
        { "mids full CW, DARK, BROAD, lean LOW",     mk (.5f, 1.f, 0, 0, 0) },
        { "mids full CCW, DARK, BROAD",              mk (.5f, 0.f, 0, 0, 0) },
        { "mids CW, tilt CW, DARK, LOW, BROAD",      mk (1.f, 1.f, 0, 0, 0) },
        { "mids CW, tilt CW, DARK, HIGH, BROAD",     mk (1.f, 1.f, 0, 1, 0) },
        { "mids CW, tilt CW, BRIGHT, HIGH, FOCUSED", mk (1.f, 1.f, 1, 1, 1) },
        { "tilt CCW only (mids .5)",                 mk (0.f, .5f, 0, 0, 0) },
    };
    std::printf ("%-42s", "Hz:"); for (double f : fs) std::printf ("%7.0f", f); std::printf ("\n");
    for (auto& c : cases)
    {
        auto b = kw::makeBank (c.s, sr);
        std::printf ("%-42s", c.name);
        for (double f : fs) std::printf ("%7.2f", kw::magnitudeDb (b, f, sr));
        std::printf ("\n");
    }
    return 0;
}
