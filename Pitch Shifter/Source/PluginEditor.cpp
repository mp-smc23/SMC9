#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
PitchShifterAudioProcessorEditor::PitchShifterAudioProcessorEditor(PitchShifterAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p), waveformS(p.waveformBufferServiceS),
      waveformT(p.waveformBufferServiceT), waveformN(p.waveformBufferServiceN), waveformOut(p.waveformBufferServiceOut),
      spectrumS(p.spectrumBufferServiceS), spectrumT(p.spectrumBufferServiceT), spectrumN(p.spectrumBufferServiceT),
      pitchShiftAttachment{audioProcessor.getPitchShiftParam(), pitchShiftSlider},
      boundsSinesAttachment{audioProcessor.getBoundsSinesParam(), boundsSinesSlider},
      boundsTransientsAttachment{audioProcessor.getBoundsTransientsParam(), boundsTransientsSlider},
      fftSizeAttachment{audioProcessor.getFFTSizeParam(), fftSizeSlider} {

    setSize(650, 420);

    styleLabel(waveformSLabel, "S");
    styleLabel(waveformTLabel, "T");
    styleLabel(waveformNLabel, "N");
    styleLabel(waveformOutLabel, "Output");
    styleLabel(pitchShiftLabel, "Pitch Shift");
    styleLabel(boundsSinesLabel, "Bounds S");
    styleLabel(fftSizeLabel, "FFT Size");
    styleLabel(boundsTransientsLabel, "Bounds T");

    pitchShiftSlider.setRange(audioProcessor.pitchShiftMin, audioProcessor.pitchShiftMax);
    pitchShiftSlider.setTextValueSuffix(" st");
    styleSlider(pitchShiftSlider);
    pitchShiftLabel.attachToComponent(&pitchShiftSlider, false);

    boundsSinesSlider.setRange(audioProcessor.minBounds, audioProcessor.maxBounds);
    styleSlider(boundsSinesSlider);
    boundsSinesLabel.attachToComponent(&boundsSinesSlider, false);

    boundsTransientsSlider.setRange(audioProcessor.minBounds, audioProcessor.maxBounds);
    styleSlider(boundsTransientsSlider);
    boundsTransientsLabel.attachToComponent(&boundsTransientsSlider, false);

    fftSizeSlider.setRange(0, 3);
    styleSlider(fftSizeSlider);
    fftSizeLabel.attachToComponent(&fftSizeSlider, false);

    styleWaveforms();

    addAndMakeVisibleComponents();
}

PitchShifterAudioProcessorEditor::~PitchShifterAudioProcessorEditor() {}

void PitchShifterAudioProcessorEditor::styleLabel(juce::Label &label, juce::String text) {
    label.setFont(juce::FontOptions(14.f));
    label.setJustificationType(juce::Justification::centred);
    label.setText(text, juce::dontSendNotification);
}

void PitchShifterAudioProcessorEditor::styleSlider(juce::Slider &slider) {
    slider.setColour(juce::Slider::ColourIds::rotarySliderFillColourId, widgetBgColour);
    slider.setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId, sliderUnusedColour);
    slider.setColour(juce::Slider::ColourIds::thumbColourId, mainColour);
    slider.setColour(juce::Slider::ColourIds::textBoxOutlineColourId, transparent);
}

void PitchShifterAudioProcessorEditor::styleWaveforms() {
    waveformS.setWaveformColour(waveformSColour);
    waveformT.setWaveformColour(waveformTColour);
    waveformN.setWaveformColour(waveformNColour);
    waveformOut.setWaveformColour(mainColour);

    waveformS.setCornerRadius(15);
    waveformT.setCornerRadius(15);
    waveformN.setCornerRadius(15);
    waveformOut.setCornerRadius(30);

    waveformS.setAxesColour(backgroundColour);
    waveformT.setAxesColour(backgroundColour);
    waveformN.setAxesColour(backgroundColour);
    waveformOut.setAxesColour(backgroundColour);

    waveformS.setBackgroundColour(widgetBgColour);
    waveformT.setBackgroundColour(widgetBgColour);
    waveformN.setBackgroundColour(widgetBgColour);
    waveformOut.setBackgroundColour(widgetBgColour);
}

void PitchShifterAudioProcessorEditor::addAndMakeVisibleComponents() {
    addAndMakeVisible(&pitchShiftSlider);
    addAndMakeVisible(&boundsSinesSlider);
    addAndMakeVisible(&boundsTransientsSlider);
    addAndMakeVisible(&fftSizeSlider);

    addAndMakeVisible(&waveformS);
    addAndMakeVisible(&waveformT);
    addAndMakeVisible(&waveformN);
    addAndMakeVisible(&waveformOut);

    addAndMakeVisible(&spectrumS);
    addAndMakeVisible(&spectrumT);
    addAndMakeVisible(&spectrumN);

    addAndMakeVisible(&waveformSLabel);
    addAndMakeVisible(&waveformTLabel);
    addAndMakeVisible(&waveformNLabel);
    addAndMakeVisible(&waveformOutLabel);

    addAndMakeVisible(&pitchShiftLabel);
    addAndMakeVisible(&boundsSinesLabel);
    addAndMakeVisible(&boundsTransientsLabel);
    addAndMakeVisible(&fftSizeLabel);
}

//==============================================================================
void PitchShifterAudioProcessorEditor::paint(juce::Graphics &g) {
    // You can add your drawing code here!
    // Paint the interface background
    g.fillAll(backgroundColour);
}

void PitchShifterAudioProcessorEditor::resized() {
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    waveformS.setBounds(75, 50, 150, 50);
    waveformSLabel.setBounds(75, 28, 150, 20);

    waveformT.setBounds(250, 50, 150, 50);
    waveformTLabel.setBounds(250, 28, 150, 20);

    waveformN.setBounds(425, 50, 150, 50);
    waveformNLabel.setBounds(425, 28, 150, 20);

    waveformOut.setBounds(75, 150, 500, 90);
    waveformOutLabel.setBounds(75, 128, 500, 20);

    pitchShiftSlider.setBounds(110, 285, 100, 100);
    boundsSinesSlider.setBounds(220, 285, 100, 100);
    boundsTransientsSlider.setBounds(330, 285, 100, 100);
    fftSizeSlider.setBounds(440, 285, 100, 100);
}
