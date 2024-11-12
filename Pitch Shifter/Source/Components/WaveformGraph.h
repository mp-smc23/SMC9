#pragma once
#include <JuceHeader.h>
#include "AudioVisualizer.h"
#include "../Services/WaveformBufferQueueService.h"

namespace components
{
    class WaveformGraph final : public juce::Component, private juce::Timer
    {
    public:
        WaveformGraph(std::shared_ptr<services::WaveformBufferQueueService> waveformBufferQueueService);
        ~WaveformGraph() override;
        [[nodiscard]] std::shared_ptr<AudioVisualizer> getWaveform() const { return waveform; }

        void setRepaintRate(int frequencyInHz);
        void paint(juce::Graphics& g) override;
        void resized() override;
    private:
        [[nodiscard]] std::shared_ptr<AudioVisualizer> waveformSetUp(juce::Colour colour) const;

        const int minWaveformLength = 256;
        const int defaultWaveformLength = 1024;
        const int maxWaveformLength = 2048;
        int curWaveformLength = 1024;

        int curSamplesPerBlock = 96;

        std::shared_ptr<AudioVisualizer> waveform;

        std::shared_ptr<services::WaveformBufferQueueService> waveformBufferQueueService;

        void timerCallback() override;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformGraph)

    };
}
