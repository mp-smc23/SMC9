/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
//#include "External/shift-stretch.h"
#include "External/signalsmith-stretch.h"
#include "DSP/STN/decomposeSTN.h"
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
    
    std::shared_ptr<services::SpectrumBufferQueueService> spectrumBufferServiceS;
    std::shared_ptr<services::SpectrumBufferQueueService> spectrumBufferServiceT;
    std::shared_ptr<services::SpectrumBufferQueueService> spectrumBufferServiceN;
    
    
    private:
    //==============================================================================
    void getParametersValues();
    
    //==============================================================================
    
    juce::AudioParameterChoice* pitchTypeParam;
    
    juce::AudioParameterFloat* pitchShiftParam;
    juce::SmoothedValue<float> pitchShiftSmoothing; // TODO use smoothing
    
    float pitchShift{1.f};
    const float pitchShiftMin{0.5f};
    const float pitchShiftMax{2.f};
    
    const double pitchBlockMs{50.};
    
    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;
    
    signalsmith::stretch::SignalsmithStretch<float> stretch;
    dsp::DecomposeSTN decomposeSTN;
    
    juce::AudioBuffer<float> abS;
    juce::AudioBuffer<float> abT;
    juce::AudioBuffer<float> abN;

    std::vector<float *> outputPointers;
    std::vector<std::vector<float>> outputBuffer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchShifterAudioProcessor)
};
