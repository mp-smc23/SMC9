/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include "Components/SpectrumGraph.h"
#include "Components/WaveformGraph.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

//==============================================================================
/**
 */
class PitchShifterAudioProcessorEditor : public juce::AudioProcessorEditor {
  public:
    PitchShifterAudioProcessorEditor(PitchShifterAudioProcessor &);
    ~PitchShifterAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    void addAndMakeVisibleComponents();
    void styleLabel(juce::Label &label, juce::String text);
    void styleSlider(juce::Slider &slider);
    void styleWaveforms();

    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PitchShifterAudioProcessor &audioProcessor;
    components::WaveformGraph waveformS;
    components::WaveformGraph waveformT;
    components::WaveformGraph waveformN;
    components::WaveformGraph waveformOut;

    components::SpectrumGraph spectrumS;
    components::SpectrumGraph spectrumT;
    components::SpectrumGraph spectrumN;

    juce::Slider pitchShiftSlider{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow};
    juce::SliderParameterAttachment pitchShiftAttachment;
    juce::Label pitchShiftLabel;

    juce::Slider boundsSinesSlider{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow};
    juce::SliderParameterAttachment boundsSinesAttachment;
    juce::Label boundsSinesLabel;

    juce::Slider boundsTransientsSlider{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow};
    juce::SliderParameterAttachment boundsTransientsAttachment;
    juce::Label boundsTransientsLabel;

    juce::Slider fftSizeSlider{juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow};
    juce::SliderParameterAttachment fftSizeAttachment;
    juce::Label fftSizeLabel;

    juce::Label waveformSLabel;
    juce::Label waveformTLabel;
    juce::Label waveformNLabel;
    juce::Label waveformOutLabel;

    // ==== colors ====
    const juce::Colour widgetBgColour{245, 245, 245};
    const juce::Colour sliderUnusedColour{180, 180, 180};
    const juce::Colour backgroundColour{72, 72, 72};
    const juce::Colour transparent = juce::Colour(0x00000000);
    const juce::Colour waveformSColour{113, 168, 170};
    const juce::Colour waveformTColour{111, 146, 191};
    const juce::Colour waveformNColour{83, 157, 255};
    const juce::Colour mainColour{255, 196, 84};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchShifterAudioProcessorEditor)
};
