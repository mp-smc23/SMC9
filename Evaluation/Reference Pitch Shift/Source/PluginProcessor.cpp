/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ReferencePitchShiftAudioProcessor::ReferencePitchShiftAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    addParameter(pitchShiftParam = new juce::AudioParameterInt({"Pitch Shift", 1}, "Pitch Shift", -12, 12, 0));
    pitchShiftSmoothing = juce::SmoothedValue(0.f);
}

ReferencePitchShiftAudioProcessor::~ReferencePitchShiftAudioProcessor()
{
}

//==============================================================================
const juce::String ReferencePitchShiftAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ReferencePitchShiftAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ReferencePitchShiftAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ReferencePitchShiftAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ReferencePitchShiftAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ReferencePitchShiftAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ReferencePitchShiftAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ReferencePitchShiftAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ReferencePitchShiftAudioProcessor::getProgramName (int index)
{
    return {};
}

void ReferencePitchShiftAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ReferencePitchShiftAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    pitchShiftSmoothing.reset(smoothingRate);
    const auto blockSamples = static_cast<int>(sampleRate * 0.001 * pitchBlockMs);
    const auto hopSizeSamples = static_cast<int>(blockSamples / 4);
    stretch.configure(1, blockSamples, hopSizeSamples);
    
    outputSinesPtrs.resize(1);
    outputSinesBuf.resize(1);
    for(auto& ab : outputSinesBuf){
        ab.resize(samplesPerBlock);
    }
    
    interpolator.reset();
    DBG("getBaseLatency: " + juce::String(interpolator.getBaseLatency()));
}

void ReferencePitchShiftAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ReferencePitchShiftAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void ReferencePitchShiftAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    pitchShiftSmoothing.setTargetValue(std::powf(2.f, pitchShiftParam->get() / 12.f));
    pitchShift = pitchShiftSmoothing.getNextValue();
//    stretch.setTransposeFactor(pitchShift);
    
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const auto numSamples = buffer.getNumSamples();
    
    // ===== Pitch Shifting =====
    // = Sines by signal smith =
//    setLatencySamples(stretch.inputLatency() + stretch.outputLatency());
    outputSinesPtrs[0] = outputSinesBuf[0].data();
//    
//    stretch.process(buffer.getArrayOfReadPointers(), numSamples, outputSinesPtrs.data(), numSamples);ś

    
    // Interpolation
    setLatencySamples(interpolator.getBaseLatency());
    [[maybe_unused]] auto _ = interpolator.process(pitchShift, buffer.getReadPointer(0), outputSinesPtrs[0], numSamples, numSamples, 1);
    buffer.copyFrom(0, 0, outputSinesPtrs[0], numSamples);
    buffer.copyFrom (1, 0, buffer.getReadPointer(0), buffer.getNumSamples());
}

//==============================================================================
bool ReferencePitchShiftAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ReferencePitchShiftAudioProcessor::createEditor()
{
    return new ReferencePitchShiftAudioProcessorEditor (*this);
}

//==============================================================================
void ReferencePitchShiftAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ReferencePitchShiftAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ReferencePitchShiftAudioProcessor();
}
