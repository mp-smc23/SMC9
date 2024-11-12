#pragma once

#include <JuceHeader.h>
#include "../Services/SpectrumBufferQueueService.h"

namespace components
{
    class SpectrumGraph final : public juce::Component, juce::Timer
    {
    public:
        explicit SpectrumGraph(const std::shared_ptr<services::SpectrumBufferQueueService>& spectrumBufferQueueService);
        ~SpectrumGraph() override;

        void pushBuffer(const std::vector<std::vector<float>>& buffers);
        void paint(juce::Graphics&) override;
        void timerCallback() override;
        void resized() override;

        void setBandFreqTolerance(int tolerance);

    private:
        std::shared_ptr<services::SpectrumBufferQueueService> spectrumBufferQueueService;
        struct FrequencyData
        {
            float frequency;
            int index;
            float x;
        };

        int bandFreqTolerance{ 0 };

        void pushNextSampleIntoFifo(float sample) noexcept;
        void drawNextFrameOfSpectrum();
        void drawFrame(juce::Graphics& g) const;
        void getPartOfSpectrumAsPath(juce::Path& path, const FrequencyData& startFrequencyData, const FrequencyData& endFrequencyData) const;
        void drawOutline(juce::Graphics& g, const juce::Path& p) const;
        void drawFill(juce::Graphics& g, juce::Path p) const;

        [[nodiscard]] float getXFromIndex(int index) const;
        [[nodiscard]] float getXFromFrequency(float freq) const;
        [[nodiscard]] int getIndexFromFrequency(float freq) const;
        
        void setSampleRate(double sampleRate);
        
        FrequencyData lowFreq;
        FrequencyData highFreq;

        juce::Colour activeFillColour;
        juce::Colour inactiveFillColour;
        juce::Colour backgroundColour;
        juce::Colour borderColour;
        juce::ColourGradient activeGradientFill;
        juce::ColourGradient inactiveGradientFill;

        juce::AffineTransform transform;

        int fifoIndex{ 0 };
        bool nextFFTBlockReady{ false };
        float sampleRate{ 44100.f };

        const int fftOrder{ 11 };
        const int fftSize{ 1 << fftOrder };
        const int scopeSize{ 1024 };

        const float maxFrequency = 20000.f;

        juce::dsp::FFT forwardFFT{ fftOrder };
        juce::dsp::WindowingFunction<float> window{ static_cast<size_t>(fftSize), juce::dsp::WindowingFunction<float>::hann };

        float *scopeData;
        float *fifo;
        float *fftData;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumGraph)
    };
}
