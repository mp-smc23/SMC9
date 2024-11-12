#include "SpectrumBufferQueueService.h"

void services::SpectrumBufferQueueService::insertBuffers(juce::AudioBuffer<float>& buffer)
{
    buffers.try_enqueue(getSampleArrayFromAudioBuffer(buffer));
}

std::vector<float> services::SpectrumBufferQueueService::getSampleArrayFromAudioBuffer(const juce::AudioBuffer<float>& buffer)
{
    std::vector<float> audioBuffer;

    audioBuffer.reserve(buffer.getNumSamples());
    for (auto j = 0; j < buffer.getNumSamples(); j++)
    {
        audioBuffer.push_back(buffer.getSample(0, j));
    }

    return audioBuffer;
}

std::vector<std::vector<float>> services::SpectrumBufferQueueService::getBuffers()
{
    std::vector<std::vector<float>> returnBuffers;
    returnBuffers.emplace_back(std::vector<float>());

    std::vector<float> buffer;
    while (buffers.try_dequeue(buffer))
    {
        returnBuffers.push_back(buffer);
    }

    return returnBuffers;
}
