#include "PluginProcessor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout KwProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID{"tilt", 1}, "Tilt",
        juce::NormalisableRange<float>(0.f, 1.f, 0.001f), 0.5f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID{"mids", 1}, "Mids",
        juce::NormalisableRange<float>(0.f, 1.f, 0.001f), 0.5f));

    p.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID{"volume", 1}, "Volume (dB)",
        juce::NormalisableRange<float>(-12.f, 12.f, 0.1f), 0.f));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID{"pivot", 1}, "Pivot",
        juce::StringArray{"Dark 400 Hz", "Bright 2 kHz"}, 0));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID{"lean", 1}, "Lean",
        juce::StringArray{"Low", "High"}, 0));

    p.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID{"width", 1}, "Width",
        juce::StringArray{"Broad", "Focused"}, 0));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID{"bypassTilt", 1}, "Bypass Tilt", false));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID{"bypassMids", 1}, "Bypass Mids", false));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID{"bypassLean", 1}, "Bypass Lean", false));

    p.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID{"bypass", 1}, "Bypass", false));

    return { p.begin(), p.end() };
}

KwProcessor::KwProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KW_EQ", createLayout())
{
}

bool KwProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& mainIn  = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();
    return mainIn == mainOut && (mainIn == juce::AudioChannelSet::mono() || mainIn == juce::AudioChannelSet::stereo());
}

kw::EqSettings KwProcessor::readSettings() const
{
    kw::EqSettings s;
    s.tilt       = tiltSm.getCurrentValue();
    s.mids       = midsSm.getCurrentValue();
    s.pivot      = (int) apvts.getRawParameterValue("pivot")->load();
    s.lean       = (int) apvts.getRawParameterValue("lean")->load();
    s.width      = (int) apvts.getRawParameterValue("width")->load();
    s.bypassTilt = apvts.getRawParameterValue("bypassTilt")->load() > 0.5f;
    s.bypassMids = apvts.getRawParameterValue("bypassMids")->load() > 0.5f;
    s.bypassLean = apvts.getRawParameterValue("bypassLean")->load() > 0.5f;
    return s;
}

void KwProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    for (auto& c : chans)
        c.reset();

    tiltSm.reset (sampleRate, 0.03);
    midsSm.reset (sampleRate, 0.03);
    volSm.reset  (sampleRate, 0.03);

    tiltSm.setCurrentAndTargetValue (apvts.getRawParameterValue("tilt")->load());
    midsSm.setCurrentAndTargetValue (apvts.getRawParameterValue("mids")->load());
    volSm.setCurrentAndTargetValue  (apvts.getRawParameterValue("volume")->load());
}

void KwProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (apvts.getRawParameterValue("bypass")->load() > 0.5f)
        return;

    tiltSm.setTargetValue (apvts.getRawParameterValue("tilt")->load());
    midsSm.setTargetValue (apvts.getRawParameterValue("mids")->load());
    volSm.setTargetValue  (apvts.getRawParameterValue("volume")->load());

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int step = 32;

    for (int start = 0; start < numSamples; start += step)
    {
        const int numThisTime = juce::jmin (step, numSamples - start);

        tiltSm.skip (numThisTime);
        midsSm.skip (numThisTime);

        const auto bank = kw::makeBank (readSettings(), sr);

        for (int ch = 0; ch < numChannels; ++ch)
            chans[ch].set (bank);

        for (int i = 0; i < numThisTime; ++i)
        {
            const float gain = juce::Decibels::decibelsToGain (volSm.getNextValue());

            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch) + start + i;
                *data = chans[ch].process (*data) * gain;
            }
        }
    }
}

juce::AudioProcessorEditor* KwProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void KwProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void KwProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
// THIS FUNCTION IS REQUIRED - do not remove
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KwProcessor();
}
