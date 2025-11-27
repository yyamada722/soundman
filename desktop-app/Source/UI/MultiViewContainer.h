#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "WaveformDisplay.h"
#include "SpectrumDisplay.h"
#include "VectorscopeDisplay.h"
#include "HistogramDisplay.h"
#include "TruePeakMeter.h"
#include "PhaseMeter.h"
#include "LoudnessMeter.h"

class MultiViewContainer : public juce::Component
{
public:
    enum class ViewType
    {
        Waveform,
        Spectrum,
        Vectorscope,
        Histogram,
        TruePeak,
        Phase,
        Loudness,
        None
    };

    struct ViewSlot
    {
        ViewType type { ViewType::None };
        juce::Component* component { nullptr };
        juce::Rectangle<int> bounds;
    };

    MultiViewContainer();
    ~MultiViewContainer() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Grid layout management
    void setGridLayout(int rows, int columns);
    void setViewInSlot(int row, int col, ViewType type);

    // Access to view components for audio callbacks
    WaveformDisplay* getWaveformDisplay() const { return waveformDisplay.get(); }
    SpectrumDisplay* getSpectrumDisplay() const { return spectrumDisplay.get(); }
    VectorscopeDisplay* getVectorscopeDisplay() const { return vectorscopeDisplay.get(); }
    HistogramDisplay* getHistogramDisplay() const { return histogramDisplay.get(); }
    TruePeakMeter* getTruePeakMeter() const { return truePeakMeter.get(); }
    PhaseMeter* getPhaseMeter() const { return phaseMeter.get(); }
    LoudnessMeter* getLoudnessMeter() const { return loudnessMeter.get(); }

private:
    void updateLayout();
    juce::Component* createViewComponent(ViewType type);

    int gridRows { 2 };
    int gridColumns { 2 };
    std::vector<ViewSlot> viewSlots;

    // View component instances
    std::unique_ptr<WaveformDisplay> waveformDisplay;
    std::unique_ptr<SpectrumDisplay> spectrumDisplay;
    std::unique_ptr<VectorscopeDisplay> vectorscopeDisplay;
    std::unique_ptr<HistogramDisplay> histogramDisplay;
    std::unique_ptr<TruePeakMeter> truePeakMeter;
    std::unique_ptr<PhaseMeter> phaseMeter;
    std::unique_ptr<LoudnessMeter> loudnessMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiViewContainer)
};
