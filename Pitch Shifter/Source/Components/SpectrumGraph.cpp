#include "SpectrumGraph.h"


components::SpectrumGraph::SpectrumGraph(const std::shared_ptr<services::SpectrumBufferQueueService>& spectrumBufferQueueService)
    : spectrumBufferQueueService(spectrumBufferQueueService)
{
    scopeData = new float[scopeSize]();
    fifo = new float[fftSize]();
    fftData = new float[2 * fftSize]();

    activeFillColour = juce::Colour(237, 169, 96);
    inactiveFillColour = juce::Colour(176, 176, 176);
    backgroundColour = juce::Colour(10, 10, 10);
    borderColour = juce::Colour(39, 39, 39);

    startTimerHz(120);
    setBufferedToImage(true);
}

components::SpectrumGraph::~SpectrumGraph()
{
    stopTimer();
    delete[] scopeData;
    delete[] fifo;
    delete[] fftData;
}

void components::SpectrumGraph::paint(juce::Graphics& g)
{
    const juce::Rectangle<float> area(0, 1, getWidth(), getHeight());

    g.setColour(backgroundColour);
    g.fillRect(juce::Rectangle<float>(getWidth(), 20));

    g.setColour(borderColour);
    g.drawHorizontalLine(0, 0, area.getWidth());

    //zaokroglenie rogow komponentu
    juce::Path p;

    p.addRoundedRectangle(area.getX(), area.getY(), area.getWidth(), area.getHeight(), 5, 5, false, false, true, true);
    g.reduceClipRegion(p);

    g.setColour(backgroundColour);
    g.fillRect(area); //insides

    drawFrame(g);
}

//components::SpectrumGraph::FrequencySegment components::SpectrumGraph::getSegmentInX(float x) const
//{
//    x = juce::jmap(x, 0.f, static_cast<float>(getWidth()), getXFromIndex(1), getXFromIndex(scopeSize - 1));
//
//    const auto lowXFreq = getXFromFrequency(lowFreq.frequency);
//    const auto highXFreq = getXFromFrequency(highFreq.frequency);
//
//    if (x < lowXFreq - bandFreqTolerance)
//    {
//        return Low;
//    }
//    if (x > lowXFreq + bandFreqTolerance && x < highXFreq - bandFreqTolerance)
//    {
//        return Mid;
//    }
//    if (x > highXFreq + bandFreqTolerance)
//    {
//        return High;
//    }
//    return activeSegment;
//}

void components::SpectrumGraph::setSampleRate(const double sampleRate)
{
    this->sampleRate = static_cast<float>(sampleRate);
    repaint();
}

void components::SpectrumGraph::setBandFreqTolerance(const int tolerance)
{
    this->bandFreqTolerance = tolerance;
}

void components::SpectrumGraph::timerCallback()
{
    setSampleRate(48000); // maybe, perhaps might be a good idea to not hardcode
    pushBuffer(spectrumBufferQueueService->getBuffers());
    if (nextFFTBlockReady)
    {
        drawNextFrameOfSpectrum();
        nextFFTBlockReady = false;
        repaint();
    }
}

void components::SpectrumGraph::pushBuffer(const std::vector<std::vector<float>>& buffers)
{
    if (!buffers.empty())
    {
        for (const auto& i : buffers)
        {
            if (const auto buffer = i.data(); buffer != nullptr)
            {
                for (const auto j : i)
                {
                    pushNextSampleIntoFifo(j);
                }
            }
        }

    }
}

void components::SpectrumGraph::pushNextSampleIntoFifo(const float sample) noexcept
{
    // if the fifo contains enough data, set a flag to say
    // that the next frame should now be rendered..
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            juce::zeromem(fftData, sizeof(fftData));
            for (auto i = 0; i < fftSize; i++)
            {
                fftData[i] = fifo[i];
            }
            nextFFTBlockReady = true;
        }

        fifoIndex = 0;
    }

    fifo[fifoIndex++] = sample;
}

void components::SpectrumGraph::drawNextFrameOfSpectrum()
{
    // first apply a windowing function to our data
    window.multiplyWithWindowingTable(fftData, fftSize);

    // then render our FFT data..
    forwardFFT.performFrequencyOnlyForwardTransform(fftData);

    const auto frequencyToSampleRateRatio = maxFrequency / sampleRate;

    juce::FloatVectorOperations::multiply(fftData, 2.0f, fftSize);

    const auto minDB = -100.0f;
    const auto maxDB = 0.0f;

    for (auto i = 0; i < scopeSize; ++i)
    {
        const int fftDataIndex = fftSize * frequencyToSampleRateRatio / scopeSize * i;
        if (fftDataIndex > fftSize / 2)
        {
            scopeData[i] = 0.f;
        }
        else
        {
            const auto level = juce::jmap(juce::jlimit(minDB, maxDB, juce::Decibels::gainToDecibels(fftData[fftDataIndex])
                - juce::Decibels::gainToDecibels(static_cast<float>(fftSize))),
                minDB, maxDB, 0.0f, 1.0f);

            scopeData[i] += level > scopeData[i] ? 0.75f * (level - scopeData[i]) : 0.2f * (level - scopeData[i]);
        }
    }
}

void components::SpectrumGraph::drawFrame(juce::Graphics& g) const
{
    juce::Path p;
    getPartOfSpectrumAsPath(p, FrequencyData{ 0., 1 }, FrequencyData{ 0.,scopeSize - 1 });
    p = p.createPathWithRoundedCorners(10);
    drawFill(g, p);
    drawOutline(g, p);
}

void components::SpectrumGraph::drawFill(juce::Graphics& g, juce::Path p) const
{
    g.setGradientFill(activeGradientFill);

    //zamkniecie
    const auto bounds = p.getBounds();
    p.lineTo(bounds.getBottomRight().getX(), 0);
    p.lineTo(bounds.getX(), 0);
    p.closeSubPath();

    g.fillPath(p, transform);
}

void components::SpectrumGraph::drawOutline(juce::Graphics& g, const juce::Path& p) const
{
    g.setColour(activeFillColour);
    g.strokePath(p, juce::PathStrokeType(1), transform);
}

float components::SpectrumGraph::getXFromIndex(const int index) const
{
    const auto binFreq = index * maxFrequency / scopeSize;
    return getXFromFrequency(binFreq);
}

float components::SpectrumGraph::getXFromFrequency(const float freq) const
{
    return log10f(freq) * (scopeSize - 1) / (log10f(maxFrequency));
}

int components::SpectrumGraph::getIndexFromFrequency(const float freq) const
{
    return static_cast<int>(scopeSize / maxFrequency * freq );
}

void components::SpectrumGraph::getPartOfSpectrumAsPath(juce::Path& path,const FrequencyData& startFrequencyData, const FrequencyData& endFrequencyData) const
{
    path.clear();
    path.preallocateSpace(4 * (endFrequencyData.index - startFrequencyData.index) + 8);

    auto x = 0.f;
    if (startFrequencyData.index == 1)
    {
        x = getXFromIndex(1.f);
    }
    else
    {
        x = startFrequencyData.x;
    }

    path.startNewSubPath(x, scopeData[startFrequencyData.index]);

    for (auto i = startFrequencyData.index + 1; i <= endFrequencyData.index - 1; ++i)
    {
        x = getXFromIndex(i);
        path.lineTo(x, scopeData[i]);
    }

    if (endFrequencyData.index == scopeSize - 1)
    {
        x = getXFromIndex(endFrequencyData.index);
    }
    else
    {
        x = endFrequencyData.x;
    }
    path.lineTo(x, scopeData[endFrequencyData.index]);
}

void components::SpectrumGraph::resized()
{
    juce::Component::resized();
    const auto area = getLocalBounds().toFloat();
    activeGradientFill = juce::ColourGradient::vertical(activeFillColour, backgroundColour, area);
    inactiveGradientFill = juce::ColourGradient::vertical(inactiveFillColour, backgroundColour, area);
    transform = juce::AffineTransform::fromTargetPoints(getXFromIndex(1), 1.0f, area.getX(), area.getY(),
        getXFromIndex(1), 0.0f, area.getX(), area.getBottom(),
        getXFromIndex(scopeSize - 1), 1.0f, area.getRight(), area.getY());
}
