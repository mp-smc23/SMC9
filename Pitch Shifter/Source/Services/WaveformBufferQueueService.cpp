#include "WaveformBufferQueueService.h"

void services::WaveformBufferQueueService::insertBuffers(const juce::AudioBuffer<float>& inputBuffer)
{
    if (inputBuffer.getNumChannels() != inputChannelsNumber)
    {
        inputChannelsNumber = inputBuffer.getNumChannels();
    }
    buffersQueue.try_enqueue(getSampleArrayFromAudioBuffer(inputBuffer));
}


std::vector<std::vector<float>> services::WaveformBufferQueueService::getSampleArrayFromAudioBuffer(const juce::AudioBuffer<float>& audioBuffer)
{
    std::vector<std::vector<float>> buffer;

    for (auto i = 0; i < audioBuffer.getNumChannels(); i++)
    {
        std::vector<float> channelBuffer;
        channelBuffer.reserve(audioBuffer.getNumSamples());
        for (auto j = 0; j < audioBuffer.getNumSamples(); j++)
        {
            channelBuffer.push_back(audioBuffer.getSample(i, j));
        }
        buffer.push_back(channelBuffer);
    }

    return buffer;
}

std::vector<std::vector<float>> services::WaveformBufferQueueService::getBuffers()
{
    std::vector<std::vector<float>> buffers;
    buffers.reserve(inputChannelsNumber);
    for (auto i = 0; i < inputChannelsNumber; i++)
    {
        buffers.emplace_back(std::vector<float>());
    }

    std::vector<std::vector<float>> buffer;
    while (buffersQueue.try_dequeue(buffer))
    {
        for (auto i = 0; i < inputChannelsNumber; i++)
        {
            for (auto& x : buffer[i])
            {
                buffers[i].push_back(x);
            }
        }
    }

    return buffers;
}
