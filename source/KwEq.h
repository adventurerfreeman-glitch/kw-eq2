#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include <juce_core/juce_core.h>

class KwEq
{
public:
    KwEq() = default;
    ~KwEq() = default;

    // Set parameters
    void setParameters(float bass, float treble, float midGain, int lowContour, int routingOrder, int highContour, bool darkBrightVoice, bool midPresence, bool midBandwidth)
    {
        m_bass = bass;
        m_treble = treble;
        m_midGain = midGain;
        m_lowContour = lowContour;
        m_routingOrder = routingOrder;
        m_highContour = highContour;
        m_darkBrightVoice = darkBrightVoice;
        m_midPresence = midPresence;
        m_midBandwidth = midBandwidth;
    }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        m_sampleRate = sampleRate;
        reset();
    }

    void reset()
    {
        // Reset filter states here if needed
    }

    float processSample(float input)
    {
        float sample = input;

        // Apply Voice bias (Dark vs Bright)
        if (m_darkBrightVoice)
            sample *= 1.05f; // Bright shift approximation
        else
            sample *= 0.95f; // Dark shift approximation

        // Process based on routing order:
        // 0 = Pre-Bax (Mid -> Bax)
        // 1 = Post-Bax (Bax -> Mid)
        // 2 = Parallel (Bax || Mid)

        if (m_routingOrder == 0)
        {
            sample = processMidStage(sample);
            sample = processBaxandall(sample);
        }
        else if (m_routingOrder == 1)
        {
            sample = processBaxandall(sample);
            sample = processMidStage(sample);
        }
        else // Parallel
        {
            float baxOut = processBaxandall(sample);
            float midOut = processMidStage(sample);
            sample = (baxOut + midOut) * 0.707f;
        }

        return sample;
    }

private:
    float processBaxandall(float in)
    {
        // Simple shelving approximation for demonstration
        float lowShelf = in * (1.0f + m_bass * 0.1f);
        float highShelf = lowShelf * (1.0f + m_treble * 0.1f);
        return highShelf;
    }

    float processMidStage(float in)
    {
        float midOut = in * (1.0f + m_midGain * 0.15f);
        if (m_midPresence)
            midOut *= 1.1f;
        return midOut;
    }

    double m_sampleRate = 44100.0;
    float m_bass = 0.0f;
    float m_treble = 0.0f;
    float m_midGain = 0.0f;
    int m_lowContour = 0;
    int m_routingOrder = 0;
    int m_highContour = 0;
    bool m_darkBrightVoice = false;
    bool m_midPresence = false;
    bool m_midBandwidth = false;
};
