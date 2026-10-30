#include "PluginEditor.h"

using namespace juce;

Colour CurveView::groupColour (int g)
{
    static const uint32 c[] = { 0xfff2b84b, 0xff5fd38d, 0xffff7a59, 0xff5aa9ff, 0xff2ec4b6, 0xffe5e86a, 0xffc58bff, 0xffff5fa2 };
    return Colour (c[g]);
}

CurveView::CurveView (KwProcessor& p) : proc (p) { startTimerHz (30); }

void CurveView::paint (Graphics& g)
{
    g.fillAll (Colour (0xff12151c));
    auto area = getLocalBounds().toFloat();
    auto plot = area.withTrimmedLeft (44).withTrimmedBottom (24).withTrimmedTop (10).withTrimmedRight (12);

    const double fMin = 20, fMax = 20000, dbMax = 9.0;
    auto xOf = [&] (double f) { return plot.getX() + (float) (std::log (f / fMin) / std::log (fMax / fMin)) * plot.getWidth(); };
    auto yOf = [&] (double db) { return plot.getCentreY() - (float) (db / dbMax) * (plot.getHeight() * 0.5f); };

    // grid
    g.setFont (11.0f);
    for (double db = -9; db <= 9.01; db += 3)
    {
        g.setColour (db == 0 ? Colour (0xff4a5263) : Colour (0xff262c38));
        g.drawHorizontalLine ((int) yOf (db), plot.getX(), plot.getRight());
        g.setColour (Colour (0xff8a93a6));
        g.drawText (String ((int) db) + " dB", Rectangle<float> (0, yOf (db) - 8, 40, 16).toNearestInt(), Justification::centredRight);
    }
    const double marks[] = { 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000 };
    const char* names[] = { "20", "50", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };
    for (int i = 0; i < 10; ++i)
    {
        g.setColour (Colour (0xff262c38));
        g.drawVerticalLine ((int) xOf (marks[i]), plot.getY(), plot.getBottom());
        g.setColour (Colour (0xff8a93a6));
        g.drawText (names[i], Rectangle<float> (xOf (marks[i]) - 20, plot.getBottom() + 3, 40, 16).toNearestInt(), Justification::centred);
    }

    const double sr = 48000.0;
    const auto s = readSettings (proc.apvts);
    const auto bank = kw::makeBank (s, sr);

    // shoulder + pivot markers
    auto marker = [&] (double f, Colour c, const String& t)
    {
        const float x = xOf (f);
        g.setColour (c.withAlpha (0.55f));
        const float dash[] = { 4.0f, 4.0f };
        g.drawDashedLine (Line<float> (x, plot.getY(), x, plot.getBottom()), dash, 2, 1.0f);
        g.setFont (10.5f);
        g.drawText (t, Rectangle<float> (x - 40, plot.getY() + 1, 80, 14).toNearestInt(), Justification::centred);
    };
    marker (kw::cfg::shoulderLowHz,  Colour (0xff8a93a6), "low shoulder");
    marker (kw::cfg::shoulderHighHz, Colour (0xff8a93a6), "high shoulder");
    marker (bank.pivotHz, Colour (0xfff2b84b), "pivot");

    auto curveFor = [&] (std::function<double (double)> fn)
    {
        Path p; bool first = true;
        for (int i = 0; i <= 400; ++i)
        {
            const double f = fMin * std::pow (fMax / fMin, i / 400.0);
            const float y = jlimit (plot.getY(), plot.getBottom(), yOf (fn (f)));
            if (first) { p.startNewSubPath (xOf (f), y); first = false; } else p.lineTo (xOf (f), y);
        }
        return p;
    };

    // total (filled to 0 dB) then per-feature lines
    Path total = curveFor ([&] (double f) { return kw::totalDb (bank, f, sr); });
    Path fill (total);
    fill.lineTo (plot.getRight(), yOf (0)); fill.lineTo (plot.getX(), yOf (0)); fill.closeSubPath();
    g.setColour (Colours::white.withAlpha (0.08f)); g.fillPath (fill);

    for (int gi = 0; gi < kw::kNumGroups; ++gi)
    {
        bool any = false;
        for (int i = 0; i < kw::kNumFilters; ++i) any |= (bank.active[(size_t) i] && bank.group[(size_t) i] == gi);
        if (! any) continue;
        g.setColour (CurveView::groupColour (gi).withAlpha (0.9f));
        g.strokePath (curveFor ([&] (double f) { return kw::groupDb (bank, gi, f, sr); }), PathStrokeType (1.6f));
    }
    g.setColour (Colours::white);
    g.strokePath (total, PathStrokeType (2.6f, PathStrokeType::curved));

    // legend
    g.setFont (11.0f);
    float lx = plot.getRight() - 6;
    for (int gi = kw::kNumGroups - 1; gi >= 0; --gi)
    {
        const String t (kw::groupName (gi));
        const float w = g.getCurrentFont().getStringWidthFloat (t) + 22;
        lx -= w;
        g.setColour (CurveView::groupColour (gi)); g.fillRoundedRectangle (lx, plot.getY() + 26, 12, 4, 2);
        g.setColour (Colour (0xffc9ceda)); g.drawText (t, Rectangle<float> (lx + 15, plot.getY() + 20, w - 15, 16).toNearestInt(), Justification::centredLeft);
    }
    g.setColour (Colours::white); g.fillRoundedRectangle (lx - 52, plot.getY() + 26, 12, 4, 2);
    g.setColour (Colour (0xffc9ceda)); g.drawText ("Total", Rectangle<float> (lx - 36, plot.getY() + 20, 40, 16).toNearestInt(), Justification::centredLeft);
}

// ------------------------------------------------------------------------------------------
KwEditor::KwEditor (KwProcessor& p) : AudioProcessorEditor (&p), proc (p), curve (p)
{
    setSize (1040, 690);
    addAndMakeVisible (curve);
    addKnob (tilt,      "tilt",      "TILT",          CurveView::groupColour (kw::gTilt));
    addKnob (mids,      "mids",      "MIDS",          CurveView::groupColour (kw::gMids));
    addKnob (leanDepth, "leanDepth", "LEAN DEPTH",    CurveView::groupColour (kw::gLean));
    addKnob (contour,   "contour",   "LOW CONTOUR",   CurveView::groupColour (kw::gContour));
    addKnob (punchK,    "punch",     "MUD>PUNCH",     CurveView::groupColour (kw::gPunch));
    addKnob (blade,     "blade",     "BLADE",         CurveView::groupColour (kw::gBlade));
    addKnob (cutK,      "cut",       "OUT CUT",       CurveView::groupColour (kw::gCut));
    addKnob (push,      "push",      "PUSH",          CurveView::groupColour (kw::gPush));
    addKnob (pushHz,    "pushHz",    "PUSH Hz",       CurveView::groupColour (kw::gPush), " Hz");
    addKnob (volume,    "volume",    "VOLUME",        Colour (0xffc9ceda), " dB");
    addCombo (pivot, "pivot", "PIVOT", { "Dark 400 Hz", "Bright 1.1 kHz" });
    addCombo (lean,  "lean",  "LEAN",  { "Low", "High" });
    addCombo (width, "width", "WIDTH", { "Broad", "Focused" });
    addToggle (onTilt,    "onTilt",    "Tilt",    CurveView::groupColour (kw::gTilt));
    addToggle (onMids,    "onMids",    "Mids",    CurveView::groupColour (kw::gMids));
    addToggle (onLean,    "onLean",    "Lean",    CurveView::groupColour (kw::gLean));
    addToggle (onContour, "onContour", "Contour", CurveView::groupColour (kw::gContour));
    addToggle (onPunch,   "onPunch",   "Punch",   CurveView::groupColour (kw::gPunch));
    addToggle (onBlade,   "onBlade",   "Blade",   CurveView::groupColour (kw::gBlade));
    addToggle (onCut,     "onCut",     "Cut",     CurveView::groupColour (kw::gCut));
    addToggle (onPush,    "onPush",    "Push",    CurveView::groupColour (kw::gPush));
    addToggle (bypassAll, "bypass",    "BYPASS ALL", Colour (0xffd9534f));
    featuresLabel.setText ("FEATURE ON / OFF  (lit = on; switch one off to hear what it does)", dontSendNotification);
    featuresLabel.setColour (Label::textColourId, Colour (0xff8a93a6)); featuresLabel.setFont (Font (12.0f));
    addAndMakeVisible (featuresLabel);
}

void KwEditor::addKnob (Knob& k, const String& id, const String& text, Colour c, const String& suffix)
{
    k.slider.setSliderStyle (Slider::RotaryVerticalDrag);
    k.slider.setTextBoxStyle (Slider::TextBoxBelow, false, 80, 18);
    k.slider.setColour (Slider::rotarySliderFillColourId, c);
    k.slider.setColour (Slider::thumbColourId, c);
    k.slider.setColour (Slider::textBoxTextColourId, Colour (0xffe8eaed));
    k.slider.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    if (suffix.isNotEmpty()) k.slider.setTextValueSuffix (suffix);
    k.label.setText (text, dontSendNotification);
    k.label.setJustificationType (Justification::centred);
    k.label.setColour (Label::textColourId, c);
    k.label.setFont (Font (12.5f, Font::bold));
    addAndMakeVisible (k.slider); addAndMakeVisible (k.label);
    k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k.slider);
}
void KwEditor::addCombo (Combo& c, const String& id, const String& text, const StringArray& items)
{
    c.box.addItemList (items, 1);
    c.label.setText (text, dontSendNotification);
    c.label.setJustificationType (Justification::centredRight);
    c.label.setColour (Label::textColourId, Colour (0xffc9ceda));
    addAndMakeVisible (c.box); addAndMakeVisible (c.label);
    c.att = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, id, c.box);
}
void KwEditor::addToggle (Toggle& t, const String& id, const String& text, Colour c)
{
    t.button.setButtonText (text);
    t.button.setClickingTogglesState (true);
    t.button.setColour (TextButton::buttonColourId, Colour (0xff232834));
    t.button.setColour (TextButton::buttonOnColourId, c.withAlpha (0.85f));
    t.button.setColour (TextButton::textColourOffId, Colour (0xff8a93a6));
    t.button.setColour (TextButton::textColourOnId, Colour (0xff10131a));
    addAndMakeVisible (t.button);
    t.att = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, t.button);
}

void KwEditor::paint (Graphics& g) { g.fillAll (Colour (0xff0f1115)); }

void KwEditor::resized()
{
    auto r = getLocalBounds();
    curve.setBounds (r.removeFromTop (330).reduced (8, 8));
    auto row = r.removeFromTop (190).reduced (8, 0);
    Knob* ks[] = { &tilt, &mids, &leanDepth, &contour, &punchK, &blade, &cutK, &push, &pushHz, &volume };
    const int w = row.getWidth() / 10;
    for (auto* k : ks)
    {
        auto c = row.removeFromLeft (w);
        k->label.setBounds (c.removeFromTop (20));
        k->slider.setBounds (c.reduced (6, 0));
    }
    auto crow = r.removeFromTop (44).reduced (16, 6);
    Combo* cs[] = { &pivot, &lean, &width };
    for (auto* c : cs)
    {
        auto cell = crow.removeFromLeft (310);
        c->label.setBounds (cell.removeFromLeft (60));
        c->box.setBounds (cell.reduced (8, 0).withWidth (170));
    }
    featuresLabel.setBounds (r.removeFromTop (20).reduced (16, 0));
    auto frow = r.removeFromTop (46).reduced (16, 6);
    Toggle* ts[] = { &onTilt, &onMids, &onLean, &onContour, &onPunch, &onBlade, &onCut, &onPush };
    for (auto* t : ts) { t->button.setBounds (frow.removeFromLeft (96).reduced (4, 0)); }
    bypassAll.button.setBounds (frow.removeFromRight (140).reduced (4, 0));
}
