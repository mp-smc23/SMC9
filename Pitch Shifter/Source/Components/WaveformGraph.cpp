
#include "WaveformGraph.h"

components::WaveformGraph::WaveformGraph(std::shared_ptr<services::WaveformBufferQueueService> waveformBufferQueueService)
    : waveformBufferQueueService(std::move(waveformBufferQueueService))
{
    waveform = std::shared_ptr<AudioVisualizer>(waveformSetUp(juce::Colour(static_cast<juce::uint8>(255), 255, 255, 0.8f)));
    addAndMakeVisible(waveform.get());

    setBufferedToImage(true);
    setRepaintRate(120);
}

components::WaveformGraph::~WaveformGraph()
{
    stopTimer();
}

std::shared_ptr<components::AudioVisualizer> components::WaveformGraph::waveformSetUp(const juce::Colour colour) const
{
    auto waveform = std::make_shared<AudioVisualizer>(maxWaveformLength, curSamplesPerBlock);
    waveform->setTopLeftPosition(0, 0);
    waveform->setWaveformColour(colour);
    waveform->setWaveformLength(defaultWaveformLength);
    return waveform;
}


juce::Rectangle<int> components::WaveformGraph::getCurrentScreenBounds()
{
    return getScreenBounds();
}

void components::WaveformGraph::paint(juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const auto h = r.getHeight();
    const auto w = r.getWidth();
    constexpr auto segments = 10;

    const juce::Rectangle<float> area(0, 0, w, h);
    juce::Path p;

    p.addRoundedRectangle(area, 5);
    g.reduceClipRegion(p);

    g.setColour(juce::Colour(10, 10, 10));
    g.fillRect(area); //insides

    g.setColour(juce::Colour(39, 39, 39));

    for (auto i = 0; i < segments; i++)
    {
        g.drawVerticalLine(static_cast<int>(w / segments * (i + 1)), 0, h);
    }

    for (auto i = 0; i < segments; i++)
    {
        g.drawHorizontalLine(static_cast<int>(h / segments * (i + 1)), 0, w);
    }
}


void components::WaveformGraph::resized()
{
    const auto rWidth = getWidth();
    const auto rHeight = getHeight();
    waveform->setSize(rWidth, rHeight);
}

void components::WaveformGraph::setRepaintRate(const int frequencyInHz)
{
    startTimerHz(frequencyInHz);
}


void components::WaveformGraph::timerCallback()
{
    waveform->pushBuffer(waveformBufferQueueService->getBuffers());
    waveform->repaint();
}
