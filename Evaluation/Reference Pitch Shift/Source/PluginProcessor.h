/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../../libs/stretch/signalsmith-stretch.h"

//==============================================================================
/**
*/
class ReferencePitchShiftAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ReferencePitchShiftAudioProcessor();
    ~ReferencePitchShiftAudioProcessor() override;

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

private:
    //==============================================================================
    
    juce::AudioParameterInt* pitchShiftParam;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> pitchShiftSmoothing;
    
    float pitchShift{1.f};
    const double pitchBlockMs{50.};
    const int smoothingRate{10}; // number of steps to reach target value
    
    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;
    int maxLatencySTN{0};
    signalsmith::stretch::SignalsmithStretch<float> stretch;
    
    std::vector<float *> outputSinesPtrs;
    std::vector<std::vector<float>> outputSinesBuf;
    
    juce::WindowedSincInterpolator interpolator;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ReferencePitchShiftAudioProcessor)
};
