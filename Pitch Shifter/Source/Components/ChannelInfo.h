#pragma once
#include <JuceHeader.h>

namespace components::helpers
{
    class ChannelInfo
    {
    public:
        ChannelInfo(const int bufferSize, const int samplesPerBlock) : samplesPerBlock(samplesPerBlock)
        {
            levels.insertMultiple(-1, {}, bufferSize);
            clear();
        }

        void clear() noexcept
        {
            levels.fill({});
            value = {};
            subSample = 0;
        }

        void pushSamples(const float* inputSamples, const int num) noexcept
        {
            for (auto i = 0; i < num; ++i)
                pushSample(inputSamples[i]);
        }

        void pushSample(const float newSample) noexcept
        {
            if (--subSample <= 0)
            {
                levels.remove(0);
                //chcemy rysowac tez jak nie ma sygnalu
                if (juce::approximatelyEqual(value.getStart(), value.getEnd()) && juce::approximatelyEqual(value.getStart(), 0.f))
                {
                    value.setStart(-0.0005f);
                    value.setEnd(0.0005f);
                }

                levels.add(value);
                subSample = samplesPerBlock;
                value = juce::Range<float>(newSample, newSample);
            }
            else
            {
                value = value.getUnionWith(newSample);
            }
        }

        juce::Array<juce::Range<float>> getLevels() const noexcept { return levels; }
        void setSamplesPerBlock(const int newValue) { this->samplesPerBlock = newValue; }

    private:
        juce::Array<juce::Range<float>> levels;
        int samplesPerBlock = 64;
        juce::Range<float> value;
        std::atomic<int> subSample{ 0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelInfo)
    };
};
