#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// Live response display: the combined curve (white) and each feature's own contribution (coloured).
class CurveView : public juce::Component, private juce::Timer
{
public:
    explicit CurveView (KwProcessor& p);
    void paint (juce::Graphics&) override;
    static juce::Colour groupColour (int g);
private:
    void timerCallback() override { repaint(); }
    KwProcessor& proc;
};

class KwEditor : public juce::AudioProcessorEditor
{
public:
    explicit KwEditor (KwProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    struct Knob
    {
        juce::Slider slider; juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };
    struct Combo
    {
        juce::ComboBox box; juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
    };
    struct Toggle
    {
        juce::TextButton button;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> att;
    };
    void addKnob (Knob&, const juce::String& id, const juce::String& text, juce::Colour c, const juce::String& suffix = {});
    void addCombo (Combo&, const juce::String& id, const juce::String& text, const juce::StringArray& items);
    void addToggle (Toggle&, const juce::String& id, const juce::String& text, juce::Colour c);

    KwProcessor& proc;
    CurveView curve;
    Knob tilt, mids, leanDepth, contour, cutK, push, pushHz, volume;
    Combo pivot, lean, width;
    Toggle onTilt, onMids, onLean, onContour, onCut, onPush, bypassAll;
    juce::Label featuresLabel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KwEditor)
};
