/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
PitchShifterAudioProcessorEditor::PitchShifterAudioProcessorEditor(PitchShifterAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p), waveformS(p.waveformBufferServiceS),
      waveformT(p.waveformBufferServiceT), waveformN(p.waveformBufferServiceN) {
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize(800, 500);
}

PitchShifterAudioProcessorEditor::~PitchShifterAudioProcessorEditor() {}

//==============================================================================
void PitchShifterAudioProcessorEditor::paint(juce::Graphics &g) {
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // these define the parameters of our slider object
    // this function adds the slider to the editor

    addAndMakeVisible(&waveformS);
    addAndMakeVisible(&waveformT);
    addAndMakeVisible(&waveformN);
}

void PitchShifterAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    waveformS.setBounds(50, 50, 700, 100);
    waveformT.setBounds(50, 200, 700, 100);
    waveformN.setBounds(50, 350, 700, 100);
}
