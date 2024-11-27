#include "AudioVisualizer.h"

components::AudioVisualizer::AudioVisualizer(int maxWaveformLength, int samplesPerBlock)
    : channels(std::make_unique<helpers::ChannelInfo>(maxWaveformLength,samplesPerBlock)),
    waveformColour(juce::Colours::white),
    maxWaveformLength(maxWaveformLength)
{
    setBufferedToImage(true);
}

void components::AudioVisualizer::pushBuffer(const std::vector<std::vector<float>>& bufferToPush) const
{
    if (!bufferToPush.empty())
    {
        const auto bufferPtr = bufferToPush.at(0).data();
        if (bufferToPush.at(0).data() != nullptr)
        {
            const auto numSamples = static_cast<int>(bufferToPush[0].size());
            const auto numChannels = static_cast<int>(bufferToPush.size());

            if (numSamples == 0 || numChannels == 0)
            {
                return;
            }

            juce::AudioBuffer<float> waveformBuffer(1, numSamples);
            waveformBuffer.clear();
            waveformBuffer.copyFrom(0, 0, bufferPtr, numSamples);
            const auto res = waveformBuffer.getWritePointer(0);
            for (auto i = 1; i < numChannels; i++)
            {
                for (auto j = 0; j < numSamples; j++)
                {
                    if (abs(res[j]) < abs(bufferToPush[i][j]))
                    {
                        res[j] = bufferToPush[i][j];
                    }
                }
            }
            channels->pushSamples(waveformBuffer.getReadPointer(0), numSamples);
        }
    }
}

void components::AudioVisualizer::paint(juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    const auto area = r.reduced(0.f,r.getHeight()*0.1f);
    g.setColour(waveformColour);
    
    juce::Path p;
    getWaveformAsPath(p, channels->getLevels());

    juce::Path p2;
    p2.addRoundedRectangle(r, radius);
    g.reduceClipRegion(p2);
    
    g.fillPath(p, juce::AffineTransform::fromTargetPoints(0.0f, -1.0f, area.getX(), area.getY(),
        0.0f, 1.0f, area.getX(), area.getBottom(),
        static_cast<float>(currentWaveformLength), -1.0f, area.getRight(), area.getY()));
}

void components::AudioVisualizer::getWaveformAsPath(juce::Path& path, const juce::Array<juce::Range<float>>& levels) const
{
    path.preallocateSpace(4 * currentWaveformLength + 8);
    const auto start = maxWaveformLength - currentWaveformLength;
    for (int i = 0; i < currentWaveformLength; ++i)
    {
        const auto level = -(levels[start + i].getEnd());

        if (i == 0)
            path.startNewSubPath(0, level);
        else
            path.lineTo(static_cast<float>(i), level);
    }

    for (int i = maxWaveformLength; --i >= start;)
    {
        path.lineTo(static_cast<float>(i - start), -(levels[i].getStart()));
    }

    path.closeSubPath();
}

void components::AudioVisualizer::setWaveformLength(const int newLength)
{
    currentWaveformLength = newLength;
}

void components::AudioVisualizer::clear() const
{
    channels->clear();
}

void components::AudioVisualizer::setWaveformColour(const juce::Colour waveformColour) noexcept
{
    this->waveformColour = waveformColour;
    repaint();
}



