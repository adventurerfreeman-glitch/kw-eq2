#include "PluginProcessor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout KwProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "tilt", 1 }, "Tilt",
                 juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mids", 1 }, "Mids",
                 juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "volume", 1 }, "Volume (dB)",
                 juce::NormalisableRange<float> (-12.f, 12.f, 0.1f), 0.f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "pivot", 1 }, "Pivot",
                 juce::StringArray { "Dark 550 Hz", "Bright 1.2 kHz" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "lean", 1 }, "Lean",
                 juce::StringArray { "Low", "High" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "width", 1 }, "Width",
                 juce::StringArray { "Broad", "Focused" }, 0));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "bypass", 1 }, "Bypass", false));
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

kw::EqSettings KwProcessor::readSettings() const
{
    kw::EqSettings s;
    s.tilt  = tiltSm.getCurrentValue();
    s.mids  = midsSm.getCurrentValue();
    s.pivot = (int) apvts.getRawParameterValue ("pivot")->load();
    s.lean  = (int) apvts.getRawParameterValue ("lean")->load();
    s.width = (int) apvts.getRawParameterValue ("width")->load();
    return s;
}

void KwProcessor::prepareToPlay (double rate, int)
{
    sr = rate;
    for (auto& c : chans) c.reset();
    tiltSm.reset (rate, 0.03); midsSm.reset (rate, 0.03); volSm.reset (rate, 0.03);
    tiltSm.setCurrentAndTargetValue (apvts.getRawParameterValue ("tilt")->load());
    midsSm.setCurrentAndTargetValue (apvts.getRawParameterValue ("mids")->load());
    volSm.setCurrentAndTargetValue (apvts.getRawParameterValue ("volume")->load());
}

void KwProcessor::processBlock (juce::AudioBuffer<float>& buf, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (apvts.getRawParameterValue ("bypass")->load() > 0.5f) return;

    tiltSm.setTargetValue (apvts.getRawParameterValue ("tilt")->load());
    midsSm.setTargetValue (apvts.getRawParameterValue ("mids")->load());
    volSm.setTargetValue  (apvts.getRawParameterValue ("volume")->load());

    const int n = buf.getNumSamples(), nch = juce::jmin (buf.getNumChannels(), 2);
    // Update the filters in short sub-blocks so knob moves are smooth.
    const int step = 32;
    for (int start = 0; start < n; start += step)
    {
        const int len = juce::jmin (step, n - start);
        tiltSm.skip (len); midsSm.skip (len);
        const auto bank = kw::makeBank (readSettings(), sr);
        for (int ch = 0; ch < nch; ++ch) chans[ch].set (bank);
        for (int i = 0; i < len; ++i)
        {
            const float gain = juce::Decibels::decibelsToGain (volSm.getNextValue());
            for (int ch = 0; ch < nch; ++ch)
            {
                float* d = buf.getWritePointer (ch) + start + i;
                *d = chans[ch].process (*d) * gain;
            }
        }
    }
}

juce::AudioProcessorEditor* KwProcessor::createEditor() { return new juce::GenericAudioProcessorEditor (*this); }

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
