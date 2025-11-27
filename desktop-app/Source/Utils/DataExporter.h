#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>

class DataExporter
{
public:
    struct AnalysisData
    {
        juce::String fileName;
        double sampleRate { 0.0 };
        int numChannels { 0 };
        int64_t lengthInSamples { 0 };
        double duration { 0.0 };
        int bitDepth { 0 };
        juce::String format;

        // Level measurements
        float leftRMS { 0.0f };
        float leftPeak { 0.0f };
        float rightRMS { 0.0f };
        float rightPeak { 0.0f };

        // Advanced measurements
        float truePeakLeft { 0.0f };
        float truePeakRight { 0.0f };
        float phaseCorrelation { 0.0f };
        float integratedLoudness { 0.0f };
        float loudnessRange { 0.0f };
    };

    static bool exportToJSON(const AnalysisData& data, const juce::File& outputFile);
    static juce::var dataToJSON(const AnalysisData& data);
};
