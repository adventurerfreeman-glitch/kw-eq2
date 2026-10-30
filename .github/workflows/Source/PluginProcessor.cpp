#include "PluginProcessor.h"
#include "PluginEditor.h"

KWTiltLeanEQAudioProcessor::KWTiltLeanEQAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

KWTiltLeanEQAudioProcessor::~KWTiltLeanEQAudioProcessor() {}

const juce::String KWTiltLeanEQAudioProcessor::getName() const { return "KW Dual Matrix EQ"; }
bool KWTiltLeanEQAudioProcessor::acceptsMidi() const { return false; }
bool KWTiltLeanEQAudioProcessor::producesMidi() const { return false; }
bool KWTiltLeanEQAudioProcessor::isMidiEffect() const { return false; }
double KWTiltLeanEQAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int KWTiltLeanEQAudioProcessor::getNumPrograms() { return 1; }
int KWTiltLeanEQAudioProcessor::getCurrentProgram() { return 0; }
void KWTiltLeanEQAudioProcessor::setCurrentProgram (int) {}
const juce::String KWTiltLeanEQAudioProcessor::getProgramName (int) { return {}; }
void KWTiltLeanEQAudioProcessor::changeProgramName (int, const juce::String&) {}

juce::AudioProcessorValueTreeState::ParameterLayout KWTiltLeanEQAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>("bass", "Bass", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("treble", "Treble", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("midGain", "Mid Lean", juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>("lowContour", "Low Contour", juce::StringArray{"Tight", "Round", "Loose"}, 1));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("routingOrder", "Routing Order", juce::StringArray{"Pre-Bax", "Post-Bax", "Parallel"}, 0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("highContour", "High Contour", juce::StringArray{"Soft", "Sparkle", "Edge"}, 1));

    params.push_back(std::make_unique<juce::AudioParameterBool>("voice", "Voicing (Dark/Bright)", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("midPresence", "Mid Presence", false));
    params.push_back(std::make_unique<juce::AudioParameterBool>("midBandwidth", "Mid Bandwidth (Broad/Narrow)", false));

    return { params.begin(), params.end() };
}

void KWTiltLeanEQAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    m_eqEngine.prepare(sampleRate, samplesPerBlock);
}

void KWTiltLeanEQAudioProcessor::releaseResources() {}

bool KWTiltLeanEQAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void KWTiltLeanEQAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float bass = apvts.getRawParameterValue("bass")->load();
    float treble = apvts.getRawParameterValue("treble")->load();
    float midGain = apvts.getRawParameterValue("midGain")->load();
    int lowContour = (int)apvts.getRawParameterValue("lowContour")->load();
    int routingOrder = (int)apvts.getRawParameterValue("routingOrder")->load();
    int highContour = (int)apvts.getRawParameterValue("highContour")->load();
    bool voice = apvts.getRawParameterValue("voice")->load() > 0.5f;
    bool midPresence = apvts.getRawParameterValue("midPresence")->load() > 0.5f;
    bool midBandwidth = apvts.getRawParameterValue("midBandwidth")->load() > 0.5f;

    m_eqEngine.setParameters(bass, treble, midGain, lowContour, routingOrder, highContour, voice, midPresence, midBandwidth);

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = m_eqEngine.processSample(channelData[sample]);
        }
    }
}

bool KWTiltLeanEQAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* KWTiltLeanEQAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

void KWTiltLeanEQAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void KWTiltLeanEQAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KWTiltLeanEQAudioProcessor();
}
