#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

static float raw (APVTS& a, const char* id) { return a.getRawParameterValue (id)->load(); }

kw::Settings readSettings (APVTS& a)
{
    kw::Settings s;
    s.tilt = raw (a, "tilt");   s.mids = raw (a, "mids");
    s.pivot = (int) raw (a, "pivot"); s.lean = (int) raw (a, "lean"); s.width = (int) raw (a, "width");
    s.leanDepth = raw (a, "leanDepth");
    s.contour = raw (a, "contour");
    s.punch = raw (a, "punch"); s.blade = raw (a, "blade");
    s.cut = raw (a, "cut"); s.push = raw (a, "push"); s.pushHz = raw (a, "pushHz");
    s.on[kw::gTilt]    = raw (a, "onTilt")    > 0.5f;
    s.on[kw::gMids]    = raw (a, "onMids")    > 0.5f;
    s.on[kw::gLean]    = raw (a, "onLean")    > 0.5f;
    s.on[kw::gContour] = raw (a, "onContour") > 0.5f;
    s.on[kw::gPunch]   = raw (a, "onPunch")   > 0.5f;
    s.on[kw::gBlade]   = raw (a, "onBlade")   > 0.5f;
    s.on[kw::gCut]     = raw (a, "onCut")     > 0.5f;
    s.on[kw::gPush]    = raw (a, "onPush")    > 0.5f;
    s.volumeDb = raw (a, "volume");
    return s;
}

APVTS::ParameterLayout KwProcessor::createLayout()
{
    using F = juce::AudioParameterFloat; using C = juce::AudioParameterChoice; using B = juce::AudioParameterBool;
    using R = juce::NormalisableRange<float>; using ID = juce::ParameterID;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<F> (ID { "tilt", 1 },      "Tilt",       R (0.f, 1.f, 0.001f), 0.5f));
    p.push_back (std::make_unique<F> (ID { "mids", 1 },      "Mids",       R (0.f, 1.f, 0.001f), 0.5f));
    p.push_back (std::make_unique<F> (ID { "leanDepth", 1 }, "Lean Depth", R (0.f, 1.5f, 0.01f), 1.0f));
    p.push_back (std::make_unique<F> (ID { "contour", 1 },   "Low Contour", R (-1.f, 1.f, 0.001f), 0.f));
    p.push_back (std::make_unique<F> (ID { "punch", 1 },     "Mud to Punch", R (0.f, 1.f, 0.001f), 0.f));
    p.push_back (std::make_unique<F> (ID { "blade", 1 },     "Blade",      R (0.f, 1.f, 0.001f), 0.f));
    p.push_back (std::make_unique<F> (ID { "cut", 1 },       "Out Cut",    R (0.f, 1.f, 0.001f), 0.f));
    p.push_back (std::make_unique<F> (ID { "push", 1 },      "Push",       R (0.f, 1.f, 0.001f), 0.f));
    p.push_back (std::make_unique<F> (ID { "pushHz", 1 },    "Push Hz",    R (60.f, 220.f, 1.f, 0.6f), 100.f));
    p.push_back (std::make_unique<F> (ID { "volume", 1 },    "Volume (dB)", R (-12.f, 12.f, 0.1f), 0.f));
    p.push_back (std::make_unique<C> (ID { "pivot", 1 }, "Pivot", juce::StringArray { "Dark 400 Hz", "Bright 1.1 kHz" }, 0));
    p.push_back (std::make_unique<C> (ID { "lean", 1 },  "Lean",  juce::StringArray { "Low", "High" }, 0));
    p.push_back (std::make_unique<C> (ID { "width", 1 }, "Width", juce::StringArray { "Broad", "Focused" }, 0));
    p.push_back (std::make_unique<B> (ID { "onTilt", 1 },    "Tilt On",    true));
    p.push_back (std::make_unique<B> (ID { "onMids", 1 },    "Mids On",    true));
    p.push_back (std::make_unique<B> (ID { "onLean", 1 },    "Lean On",    true));
    p.push_back (std::make_unique<B> (ID { "onContour", 1 }, "Contour On", true));
    p.push_back (std::make_unique<B> (ID { "onPunch", 1 },   "Punch On",    true));
    p.push_back (std::make_unique<B> (ID { "onBlade", 1 },   "Blade On",    true));
    p.push_back (std::make_unique<B> (ID { "onCut", 1 },     "Cut On",     true));
    p.push_back (std::make_unique<B> (ID { "onPush", 1 },    "Push On",    true));
    p.push_back (std::make_unique<B> (ID { "bypass", 1 },    "Bypass All", false));
    return { p.begin(), p.end() };
}

KwProcessor::KwProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KW_EQ", createLayout())
{
}

bool KwProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in = l.getMainInputChannelSet(), out = l.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

void KwProcessor::prepareToPlay (double rate, int)
{
    sr = rate;
    for (auto& c : chans) c.reset();
    for (auto* sm : { &tiltSm, &midsSm, &contourSm, &punchSm, &bladeSm, &cutSm, &pushSm, &volSm }) sm->reset (rate, 0.03);
    tiltSm.setCurrentAndTargetValue (raw (apvts, "tilt"));       midsSm.setCurrentAndTargetValue (raw (apvts, "mids"));
    contourSm.setCurrentAndTargetValue (raw (apvts, "contour")); cutSm.setCurrentAndTargetValue (raw (apvts, "cut"));
    punchSm.setCurrentAndTargetValue (raw (apvts, "punch")); bladeSm.setCurrentAndTargetValue (raw (apvts, "blade"));
    pushSm.setCurrentAndTargetValue (raw (apvts, "push"));       volSm.setCurrentAndTargetValue (raw (apvts, "volume"));
}

void KwProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (raw (apvts, "bypass") > 0.5f) return;

    kw::Settings s = readSettings (apvts);
    tiltSm.setTargetValue (s.tilt);       midsSm.setTargetValue (s.mids);
    contourSm.setTargetValue (s.contour); cutSm.setTargetValue (s.cut);
    punchSm.setTargetValue (s.punch); bladeSm.setTargetValue (s.blade);
    pushSm.setTargetValue (s.push);       volSm.setTargetValue (s.volumeDb);

    const int n = buf.getNumSamples(), nch = juce::jmin (buf.getNumChannels(), 2);
    const int step = 32;                      // update the filters in short sub-blocks: smooth knob moves
    for (int start = 0; start < n; start += step)
    {
        const int len = juce::jmin (step, n - start);
        s.tilt = tiltSm.skip (len);   s.mids = midsSm.skip (len);   s.contour = contourSm.skip (len);
        s.punch = punchSm.skip (len); s.blade = bladeSm.skip (len);
        s.cut  = cutSm.skip (len);    s.push = pushSm.skip (len);   s.volumeDb = volSm.skip (len);
        const kw::Bank bank = kw::makeBank (s, sr);
        for (int ch = 0; ch < nch; ++ch)
        {
            float* d = buf.getWritePointer (ch) + start;
            for (int i = 0; i < len; ++i) d[i] = chans[ch].process (bank, d[i]);
        }
    }
}

juce::AudioProcessorEditor* KwProcessor::createEditor() { return new KwEditor (*this); }

void KwProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}
void KwProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KwProcessor(); }
