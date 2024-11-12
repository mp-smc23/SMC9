#pragma once

#include "JuceHeader.h"
#include "readerwritercircularbuffer.h"

namespace services
{
    class WaveformBufferQueueService
    {
    public:
        WaveformBufferQueueService() = default;

        void insertBuffers(const juce::AudioBuffer<float>& buffer);

        static std::vector<std::vector<float>> getSampleArrayFromAudioBuffer(const juce::AudioBuffer<float>& audioBuffer);

        std::vector<std::vector<float>> getBuffers();
        
    private:
        int inputChannelsNumber = 0;

        moodycamel::BlockingReaderWriterCircularBuffer<std::vector<std::vector<float>>> buffersQueue{ 10 };
    };
}
