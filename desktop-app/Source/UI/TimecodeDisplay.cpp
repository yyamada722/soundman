#include "TimecodeDisplay.h"

TimecodeDisplay::TimecodeDisplay()
{
    addAndMakeVisible(timecodeLabel);
    timecodeLabel.setFont(juce::Font(32.0f, juce::Font::bold));
    timecodeLabel.setJustificationType(juce::Justification::centred);
    timecodeLabel.setText("00:00:00.000", juce::dontSendNotification);
    timecodeLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    addAndMakeVisible(formatLabel);
    formatLabel.setText("Format:", juce::dontSendNotification);
    formatLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    formatLabel.setJustificationType(juce::Justification::centredRight);

    addAndMakeVisible(formatCombo);
    formatCombo.addItem("SMPTE (HH:MM:SS:FF)", 1);
    formatCombo.addItem("Samples", 2);
    formatCombo.addItem("Milliseconds", 3);
    formatCombo.addItem("Time (HH:MM:SS.mmm)", 4);
    formatCombo.setSelectedId(4);
    formatCombo.onChange = [this]()
    {
        int id = formatCombo.getSelectedId();
        switch (id)
        {
            case 1: currentFormat = TimecodeFormat::SMPTE; break;
            case 2: currentFormat = TimecodeFormat::Samples; break;
            case 3: currentFormat = TimecodeFormat::Milliseconds; break;
            case 4: currentFormat = TimecodeFormat::Timecode; break;
            default: currentFormat = TimecodeFormat::Timecode; break;
        }
        timecodeLabel.setText(formatTimecode(currentSamples, currentSampleRate), juce::dontSendNotification);
    };

    startTimerHz(30);
}

TimecodeDisplay::~TimecodeDisplay()
{
    stopTimer();
}

void TimecodeDisplay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff2a2a2a));

    auto bounds = getLocalBounds().reduced(10);

    // Draw border
    g.setColour(juce::Colour(0xff404040));
    g.drawRect(bounds, 2);

    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("TIMECODE", bounds.removeFromTop(25), juce::Justification::centred);
}

void TimecodeDisplay::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    bounds.removeFromTop(25); // Title space

    auto timecodeBounds = bounds.removeFromTop(50);
    timecodeLabel.setBounds(timecodeBounds);

    bounds.removeFromTop(10); // Spacing

    auto formatRow = bounds.removeFromTop(25);
    formatLabel.setBounds(formatRow.removeFromLeft(60));
    formatCombo.setBounds(formatRow.reduced(5, 0));
}

void TimecodeDisplay::setPosition(int64_t samples, double sampleRate)
{
    currentSamples = samples;
    currentSampleRate = sampleRate;

    timecodeLabel.setText(formatTimecode(currentSamples, currentSampleRate), juce::dontSendNotification);
}

void TimecodeDisplay::setFormat(TimecodeFormat format)
{
    currentFormat = format;
    timecodeLabel.setText(formatTimecode(currentSamples, currentSampleRate), juce::dontSendNotification);
}

void TimecodeDisplay::timerCallback()
{
    // Update display (will be driven by audio engine in real implementation)
}

juce::String TimecodeDisplay::formatTimecode(int64_t samples, double sampleRate)
{
    switch (currentFormat)
    {
        case TimecodeFormat::SMPTE:        return formatSMPTE(samples, sampleRate);
        case TimecodeFormat::Samples:      return formatSamples(samples);
        case TimecodeFormat::Milliseconds: return formatMilliseconds(samples, sampleRate);
        case TimecodeFormat::Timecode:     return formatTime(samples, sampleRate);
        default:                           return formatTime(samples, sampleRate);
    }
}

juce::String TimecodeDisplay::formatSMPTE(int64_t samples, double sampleRate)
{
    if (sampleRate <= 0)
        return "00:00:00:00";

    double totalSeconds = samples / sampleRate;
    int hours = static_cast<int>(totalSeconds / 3600);
    int minutes = static_cast<int>((totalSeconds - hours * 3600) / 60);
    int seconds = static_cast<int>(totalSeconds - hours * 3600 - minutes * 60);
    double fractionalSeconds = totalSeconds - static_cast<int>(totalSeconds);
    int frames = static_cast<int>(fractionalSeconds * frameRate);

    return juce::String::formatted("%02d:%02d:%02d:%02d", hours, minutes, seconds, frames);
}

juce::String TimecodeDisplay::formatSamples(int64_t samples)
{
    return juce::String(samples) + " samples";
}

juce::String TimecodeDisplay::formatMilliseconds(int64_t samples, double sampleRate)
{
    if (sampleRate <= 0)
        return "0 ms";

    double milliseconds = (samples / sampleRate) * 1000.0;
    return juce::String::formatted("%.3f ms", milliseconds);
}

juce::String TimecodeDisplay::formatTime(int64_t samples, double sampleRate)
{
    if (sampleRate <= 0)
        return "00:00:00.000";

    double totalSeconds = samples / sampleRate;
    int hours = static_cast<int>(totalSeconds / 3600);
    int minutes = static_cast<int>((totalSeconds - hours * 3600) / 60);
    int seconds = static_cast<int>(totalSeconds - hours * 3600 - minutes * 60);
    int milliseconds = static_cast<int>((totalSeconds - static_cast<int>(totalSeconds)) * 1000);

    return juce::String::formatted("%02d:%02d:%02d.%03d", hours, minutes, seconds, milliseconds);
}
