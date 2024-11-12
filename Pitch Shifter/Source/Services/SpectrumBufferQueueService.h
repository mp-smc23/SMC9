#pragma once

#include "JuceHeader.h"
#include "readerwritercircularbuffer.h"

namespace services
{
    class SpectrumBufferQueueService
    {
    public:
        SpectrumBufferQueueService() = default;

        void insertBuffers(juce::AudioBuffer<float>& buffer);

        static std::vector<float> getSampleArrayFromAudioBuffer(const juce::AudioBuffer<float>& buffer);

        std::vector<std::vector<float>> getBuffers();
    private:
        moodycamel::BlockingReaderWriterCircularBuffer<std::vector<float>> buffers { 10 };
    };
}
