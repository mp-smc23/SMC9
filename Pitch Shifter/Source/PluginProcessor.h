/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "External/signalsmith-stretch.h"
#include "DSP/STN/decomposeSTN.h"
#include "DSP/NM/NoiseMorphing.h"
#include "Services/WaveformBufferQueueService.h"
#include "Services/SpectrumBufferQueueService.h"

//==============================================================================
/**
*/
class PitchShifterAudioProcessor  : public juce::AudioProcessor
{
    public:
    //==============================================================================
    PitchShifterAudioProcessor();
    ~PitchShifterAudioProcessor() override;
    
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif
    
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    
    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    
    //==============================================================================
    const juce::String getName() const override;
    
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    
    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;
    
    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    std::shared_ptr<services::WaveformBufferQueueService> waveformBufferServiceS;
    std::shared_ptr<services::WaveformBufferQueueService> waveformBufferServiceT;
    std::shared_ptr<services::WaveformBufferQueueService> waveformBufferServiceN;
    std::shared_ptr<services::WaveformBufferQueueService> waveformBufferServiceOut;
    
    std::shared_ptr<services::SpectrumBufferQueueService> spectrumBufferServiceS;
    std::shared_ptr<services::SpectrumBufferQueueService> spectrumBufferServiceT;
    std::shared_ptr<services::SpectrumBufferQueueService> spectrumBufferServiceN;
    
    
    private:
    //==============================================================================
    void getParametersValues();
    
    //==============================================================================
    
    juce::AudioParameterChoice* fftSizeParam;
    int fftSizes[4]{512, 1024, 2048, 4096};
    
    juce::AudioParameterInt* pitchShiftParam;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pitchShiftSmoothing;
    
    juce::AudioParameterFloat* boundsSinesParam;
    juce::AudioParameterFloat* boundsTransientsParam;
    
    float pitchShift{1.f};
    const int pitchShiftMin{-12};
    const int pitchShiftMax{24};
    
    const double pitchBlockMs{50.};
    const int smoothingRate{10}; // number of steps to reach target value
    
    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;
    
    signalsmith::stretch::SignalsmithStretch<float> stretch;
    dsp::DecomposeSTN decomposeSTN;
    dsp::NoiseMorphing noiseMorphing;
    
    juce::AudioBuffer<float> abS;
    juce::AudioBuffer<float> abT;
    juce::AudioBuffer<float> abN;
       
    juce::dsp::DelayLine<float> sinesDelayLine;
    juce::dsp::DelayLine<float> transientsDelayLine;
    juce::dsp::DelayLine<float> noiseDelayLine;
    
    std::vector<float *> outputSinesPtrs;
    std::vector<std::vector<float>> outputSinesBuf;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchShifterAudioProcessor)
};
