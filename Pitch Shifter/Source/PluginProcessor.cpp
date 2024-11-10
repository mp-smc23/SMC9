/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PitchShifterAudioProcessor::PitchShifterAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
    processSpec(std::make_shared<juce::dsp::ProcessSpec>()),
    decomposeSTN(processSpec)
#endif
{
    addParameter(pitchShiftParam = new juce::AudioParameterFloat({"Pitch Shift", 1}, "Pitch Shift", pitchShiftMin, pitchShiftMax, 1.f));
//    addParameter(pitchTypeParam = new juce::AudioParameterChoice({"Pitch Type", 1}, "Pitch Type", {"Soundsmith", "SNT", "Other"}, 0));
}

PitchShifterAudioProcessor::~PitchShifterAudioProcessor()
{
}

//==============================================================================
const juce::String PitchShifterAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PitchShifterAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PitchShifterAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PitchShifterAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PitchShifterAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PitchShifterAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PitchShifterAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PitchShifterAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PitchShifterAudioProcessor::getProgramName (int index)
{
    return {};
}

void PitchShifterAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PitchShifterAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    const auto channels = getTotalNumInputChannels();

//    stretch.presetDefault(channels, sampleRate);
    const auto blockSamples = static_cast<int>(sampleRate * 0.001 * pitchBlockMs);
    const auto hopSizeSamples = static_cast<int>(blockSamples / 4);
    stretch.configure(channels, blockSamples, hopSizeSamples);
    
    setLatencySamples(stretch.inputLatency() + stretch.outputLatency());

    // query the current configuration
    DBG("Block Samples: " << stretch.blockSamples());
    DBG("Interval Samples: " << stretch.intervalSamples());
    DBG("============ LATENCY ============");
    DBG("Input: " << stretch.inputLatency() << "| Output: " << stretch.outputLatency() << "| Total: " << (stretch.inputLatency() + stretch.outputLatency()) << " | Total (ms): " << ((stretch.inputLatency() + stretch.outputLatency())/sampleRate * 1000));
    
    outputPointers.resize(channels);
    outputBuffer.resize(channels);
    for(auto& ab : outputBuffer){
        ab.resize(samplesPerBlock);
    }
    
    abS.setSize(channels, samplesPerBlock);
    abT.setSize(channels, samplesPerBlock);
    abN.setSize(channels, samplesPerBlock);
    
    if(processSpec->numChannels != channels || processSpec->maximumBlockSize != samplesPerBlock || processSpec->sampleRate != sampleRate){
        processSpec->maximumBlockSize = samplesPerBlock;
        processSpec->numChannels = channels;
        processSpec->sampleRate = sampleRate;
        
        decomposeSTN.prepare();
    }

}

void PitchShifterAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PitchShifterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PitchShifterAudioProcessor::getParametersValues(){
    pitchShift = pitchShiftParam->get();
}

void PitchShifterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const auto channels = totalNumInputChannels;
    const auto numSamples = buffer.getNumSamples();
    
    getParametersValues();
    
    decomposeSTN.process(buffer, abS, abT, abN);

    buffer.copyFrom(0, 0, abS, 0, 0, numSamples);
    buffer.addFrom(0, 0, abT, 0, 0, numSamples);
    buffer.addFrom(0, 0, abN, 0, 0, numSamples);
    
    buffer.addFrom(1, 0, abN, 0, 0, numSamples);
    
    // ===== Pitch Shifting by signal smith =====
//    for (int c = 0; c < channels; ++c) { // maybe could be just set in prepareToPlay?
//        outputPointers[c] = outputBuffer[c].data();
//    }
//    
//    stretch.setTransposeFactor(pitchShift);
//    stretch.process(buffer.getArrayOfReadPointers(), numSamples, outputPointers.data(), numSamples);
//    
//    for (int c = 0; c < channels; ++c) {
//        buffer.copyFrom(c, 0, outputPointers[c], numSamples);
//    }
}

//==============================================================================
bool PitchShifterAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PitchShifterAudioProcessor::createEditor()
{
    return new PitchShifterAudioProcessorEditor (*this);
}

//==============================================================================
void PitchShifterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PitchShifterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchShifterAudioProcessor();
}
