#pragma once
#include "JuceHeader.h"

namespace components::events
{
    class WaveformGraphEvents
    {
    public:
        virtual ~WaveformGraphEvents() = default;
        virtual juce::Rectangle<int> getCurrentScreenBounds() = 0;
    };
}
