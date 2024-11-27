
#pragma once
#include <JuceHeader.h>
#include "ChannelInfo.h"

namespace components
{
    class AudioVisualizer final : public juce::Component
    {
    public:
        AudioVisualizer(int maxWaveformLength, int samplesPerBlock);

        ~AudioVisualizer() override = default;

        void pushBuffer(const std::vector<std::vector<float>>& bufferToPush) const;
        void setWaveformColour(juce::Colour waveformColour) noexcept;
        void setWaveformLength(int bufferSize);
        void setRadius(int radius) noexcept { this->radius = radius; }
        void paint(juce::Graphics&) override;
        void clear() const;

    private:

        std::unique_ptr<helpers::ChannelInfo> channels;
        juce::Colour waveformColour;
        int radius{15};
        int maxWaveformLength;
        int currentWaveformLength;

        void getWaveformAsPath(juce::Path& path, const juce::Array<juce::Range<float>>& levels) const;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioVisualizer)
    };
}
