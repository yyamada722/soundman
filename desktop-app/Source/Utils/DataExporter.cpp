#include "DataExporter.h"

bool DataExporter::exportToJSON(const AnalysisData& data, const juce::File& outputFile)
{
    auto jsonData = dataToJSON(data);

    // Ensure parent directory exists
    outputFile.getParentDirectory().createDirectory();

    // Write to file
    auto outputStream = outputFile.createOutputStream();
    if (outputStream == nullptr)
        return false;

    juce::JSON::writeToStream(*outputStream, jsonData, true);
    return true;
}

juce::var DataExporter::dataToJSON(const AnalysisData& data)
{
    auto* root = new juce::DynamicObject();

    // File information
    auto* fileInfo = new juce::DynamicObject();
    fileInfo->setProperty("fileName", data.fileName);
    fileInfo->setProperty("format", data.format);
    fileInfo->setProperty("sampleRate", data.sampleRate);
    fileInfo->setProperty("numChannels", data.numChannels);
    fileInfo->setProperty("lengthInSamples", static_cast<juce::int64>(data.lengthInSamples));
    fileInfo->setProperty("duration", data.duration);
    fileInfo->setProperty("bitDepth", data.bitDepth);
    root->setProperty("fileInfo", juce::var(fileInfo));

    // Level measurements
    auto* levels = new juce::DynamicObject();
    levels->setProperty("leftRMS", data.leftRMS);
    levels->setProperty("leftPeak", data.leftPeak);
    levels->setProperty("rightRMS", data.rightRMS);
    levels->setProperty("rightPeak", data.rightPeak);

    // Convert to dB
    auto* levelsDB = new juce::DynamicObject();
    levelsDB->setProperty("leftRMS_dB", juce::Decibels::gainToDecibels(data.leftRMS, -96.0f));
    levelsDB->setProperty("leftPeak_dB", juce::Decibels::gainToDecibels(data.leftPeak, -96.0f));
    levelsDB->setProperty("rightRMS_dB", juce::Decibels::gainToDecibels(data.rightRMS, -96.0f));
    levelsDB->setProperty("rightPeak_dB", juce::Decibels::gainToDecibels(data.rightPeak, -96.0f));
    levels->setProperty("dB", juce::var(levelsDB));

    root->setProperty("levels", juce::var(levels));

    // Advanced measurements
    auto* advanced = new juce::DynamicObject();
    advanced->setProperty("truePeakLeft", data.truePeakLeft);
    advanced->setProperty("truePeakRight", data.truePeakRight);
    advanced->setProperty("truePeakLeft_dBTP", juce::Decibels::gainToDecibels(data.truePeakLeft, -96.0f));
    advanced->setProperty("truePeakRight_dBTP", juce::Decibels::gainToDecibels(data.truePeakRight, -96.0f));
    advanced->setProperty("phaseCorrelation", data.phaseCorrelation);
    advanced->setProperty("integratedLoudness_LUFS", data.integratedLoudness);
    advanced->setProperty("loudnessRange_LU", data.loudnessRange);
    root->setProperty("advanced", juce::var(advanced));

    // Metadata
    auto* metadata = new juce::DynamicObject();
    metadata->setProperty("exportedAt", juce::Time::getCurrentTime().toString(true, true));
    metadata->setProperty("exportVersion", "1.0");
    metadata->setProperty("application", "Soundman Desktop");
    root->setProperty("metadata", juce::var(metadata));

    return juce::var(root);
}
