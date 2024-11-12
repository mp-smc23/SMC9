/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Components/WaveformGraph.h"

//==============================================================================
/**
*/
class PitchShifterAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    PitchShifterAudioProcessorEditor (PitchShifterAudioProcessor&);
    ~PitchShifterAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PitchShifterAudioProcessor& audioProcessor;
    components::WaveformGraph waveformS;
    components::WaveformGraph waveformT;
    components::WaveformGraph waveformN;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PitchShifterAudioProcessorEditor)
};
