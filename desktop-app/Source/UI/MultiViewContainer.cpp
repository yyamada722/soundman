#include "MultiViewContainer.h"

MultiViewContainer::MultiViewContainer()
{
    // Create view component instances
    waveformDisplay = std::make_unique<WaveformDisplay>();
    spectrumDisplay = std::make_unique<SpectrumDisplay>();
    vectorscopeDisplay = std::make_unique<VectorscopeDisplay>();
    histogramDisplay = std::make_unique<HistogramDisplay>();
    truePeakMeter = std::make_unique<TruePeakMeter>();
    phaseMeter = std::make_unique<PhaseMeter>();
    loudnessMeter = std::make_unique<LoudnessMeter>();

    // Set default 2x2 grid layout
    setGridLayout(2, 2);

    // Default view configuration
    setViewInSlot(0, 0, ViewType::Waveform);
    setViewInSlot(0, 1, ViewType::Spectrum);
    setViewInSlot(1, 0, ViewType::Vectorscope);
    setViewInSlot(1, 1, ViewType::Histogram);
}

MultiViewContainer::~MultiViewContainer()
{
}

void MultiViewContainer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
}

void MultiViewContainer::resized()
{
    updateLayout();
}

void MultiViewContainer::setGridLayout(int rows, int columns)
{
    gridRows = juce::jmax(1, rows);
    gridColumns = juce::jmax(1, columns);

    viewSlots.clear();
    viewSlots.resize(gridRows * gridColumns);

    updateLayout();
}

void MultiViewContainer::setViewInSlot(int row, int col, ViewType type)
{
    if (row < 0 || row >= gridRows || col < 0 || col >= gridColumns)
        return;

    int index = row * gridColumns + col;
    if (index >= viewSlots.size())
        return;

    auto& slot = viewSlots[index];

    // Remove previous component if any
    if (slot.component != nullptr)
    {
        removeChildComponent(slot.component);
        slot.component = nullptr;
    }

    slot.type = type;
    slot.component = createViewComponent(type);

    if (slot.component != nullptr)
    {
        addAndMakeVisible(slot.component);
    }

    updateLayout();
}

void MultiViewContainer::updateLayout()
{
    auto bounds = getLocalBounds();
    int slotWidth = bounds.getWidth() / gridColumns;
    int slotHeight = bounds.getHeight() / gridRows;

    for (int row = 0; row < gridRows; ++row)
    {
        for (int col = 0; col < gridColumns; ++col)
        {
            int index = row * gridColumns + col;
            if (index >= viewSlots.size())
                continue;

            auto& slot = viewSlots[index];

            int x = col * slotWidth;
            int y = row * slotHeight;

            slot.bounds = juce::Rectangle<int>(x, y, slotWidth, slotHeight).reduced(2);

            if (slot.component != nullptr)
            {
                slot.component->setBounds(slot.bounds);
            }
        }
    }
}

juce::Component* MultiViewContainer::createViewComponent(ViewType type)
{
    switch (type)
    {
        case ViewType::Waveform:    return waveformDisplay.get();
        case ViewType::Spectrum:    return spectrumDisplay.get();
        case ViewType::Vectorscope: return vectorscopeDisplay.get();
        case ViewType::Histogram:   return histogramDisplay.get();
        case ViewType::TruePeak:    return truePeakMeter.get();
        case ViewType::Phase:       return phaseMeter.get();
        case ViewType::Loudness:    return loudnessMeter.get();
        case ViewType::None:
        default:                    return nullptr;
    }
}
