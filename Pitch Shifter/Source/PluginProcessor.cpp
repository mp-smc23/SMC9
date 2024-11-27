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
    decomposeSTN(processSpec),
    noiseMorphing(processSpec),
    sinesDelayLine(4096), transientsDelayLine(4096), noiseDelayLine(4096)
#endif
{
    waveformBufferServiceS = std::make_shared<services::WaveformBufferQueueService>();
    waveformBufferServiceT = std::make_shared<services::WaveformBufferQueueService>();
    waveformBufferServiceN = std::make_shared<services::WaveformBufferQueueService>();
    waveformBufferServiceOut = std::make_shared<services::WaveformBufferQueueService>();
    
    spectrumBufferServiceS = std::make_shared<services::SpectrumBufferQueueService>();
    spectrumBufferServiceT = std::make_shared<services::SpectrumBufferQueueService>();
    spectrumBufferServiceN = std::make_shared<services::SpectrumBufferQueueService>();
    
    addParameter(pitchShiftParam = new juce::AudioParameterInt({"Pitch Shift", 1}, "Pitch Shift", pitchShiftMin, pitchShiftMax, 0));
    addParameter(boundsSinesParam = new juce::AudioParameterFloat({"Bounds Sines", 1}, "Bounds Sines", 0.4f, 0.9f, 0.6f));
    addParameter(boundsTransientsParam = new juce::AudioParameterFloat({"Bounds Transients", 1}, "Bounds Transients", 0.4f, 0.9f, 0.7f));
    
    addParameter(fftSizeParam = new juce::AudioParameterChoice({"STN FFT Size", 1}, "STN FFT Size", {"512", "1024", "2048", "4096"}, 2));
    
    pitchShiftSmoothing = juce::SmoothedValue(0.f);
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
    stretch.configure(1, blockSamples, hopSizeSamples);
    
    outputSinesPtrs.resize(1);
    outputSinesBuf.resize(1);
    for(auto& ab : outputSinesBuf){
        ab.resize(samplesPerBlock);
    }
    
    abS.setSize(1, samplesPerBlock);
    abT.setSize(1, samplesPerBlock);
    abN.setSize(1, samplesPerBlock);
    
    if(processSpec->numChannels != channels || processSpec->maximumBlockSize != samplesPerBlock || processSpec->sampleRate != sampleRate){
        processSpec->maximumBlockSize = samplesPerBlock;
        processSpec->numChannels = channels;
        processSpec->sampleRate = sampleRate;
        
        decomposeSTN.prepare();
        noiseMorphing.prepare();
    }

    pitchShiftSmoothing.reset(smoothingRate);
    
    
    DBG("============ CONFIGURATION ============");
    DBG("Sample Rate: " << sampleRate);
    DBG("Samples Per Block: " << samplesPerBlock);
    DBG("Channels: " << channels);
    DBG("Stretch Block Samples: " << stretch.blockSamples());
    DBG("Stretch Interval Samples: " << stretch.intervalSamples());
    
    DBG("============ LATENCY ============");
    DBG("Input: " << stretch.inputLatency() << "| Output: " << stretch.outputLatency() << "| Total: " << (stretch.inputLatency() + stretch.outputLatency()) << " | Total (ms): " << ((stretch.inputLatency() + stretch.outputLatency()) / sampleRate * 1000));
    DBG("Decompose STN: " << decomposeSTN.getLatency());
    DBG("Noise Morphing: " << noiseMorphing.getLatency());
    
    const auto maxLatencySTN = juce::jmax(noiseMorphing.getLatency(), (stretch.inputLatency() + stretch.outputLatency()));
    const auto sinesLatency = maxLatencySTN - (stretch.inputLatency() + stretch.outputLatency());
    const auto transientsLatency = maxLatencySTN;
    const auto noiseLatency = maxLatencySTN - noiseMorphing.getLatency();
    setLatencySamples(decomposeSTN.getLatency() + maxLatencySTN);
    
    sinesDelayLine.prepare({processSpec->sampleRate, processSpec->maximumBlockSize, 1});
    sinesDelayLine.setMaximumDelayInSamples(sinesLatency);
    sinesDelayLine.setDelay(sinesLatency);
    sinesDelayLine.reset();
    
    transientsDelayLine.prepare({processSpec->sampleRate, processSpec->maximumBlockSize, 1});
    transientsDelayLine.setMaximumDelayInSamples(transientsLatency);
    transientsDelayLine.setDelay(transientsLatency);
    transientsDelayLine.reset();
    
    noiseDelayLine.prepare({processSpec->sampleRate, processSpec->maximumBlockSize, 1});
    noiseDelayLine.setMaximumDelayInSamples(noiseLatency);
    noiseDelayLine.setDelay(noiseLatency);
    noiseDelayLine.reset();
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
    pitchShiftSmoothing.setTargetValue(std::powf(2.f, pitchShiftParam->get() / 12.f));
    pitchShift = pitchShiftSmoothing.getNextValue();
    
    stretch.setTransposeFactor(pitchShift);
    noiseMorphing.setPitchShiftSemitones(pitchShiftParam->get()); // TODO test with ratio not semitones
    
    decomposeSTN.setThresholdSines(boundsSinesParam->get());
    decomposeSTN.setThresholdTransients(boundsTransientsParam->get());
    
    const auto fftSize = fftSizes[fftSizeParam->getIndex()];
    decomposeSTN.setWindowS(fftSize);
    decomposeSTN.setWindowTN(static_cast<int>(fftSize * 0.25f));
}


void PitchShifterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const auto numSamples = buffer.getNumSamples();
    
    // ===== Get Parameters =====
    getParametersValues();
    
    // ===== Decompose STN =====
    decomposeSTN.process(buffer, abS, abT, abN);
    
    // ===== Pitch Shifting =====
    // = Sines by signal smith =
    outputSinesPtrs[0] = outputSinesBuf[0].data();
    
    stretch.process(abS.getArrayOfReadPointers(), numSamples, outputSinesPtrs.data(), numSamples);
    abS.copyFrom(0, 0, outputSinesPtrs[0], numSamples);

    // = Noise =
    noiseMorphing.process(abN);
    
    // ===== Latency Handling =====
    // = Sines =
    juce::dsp::AudioBlock<float> sinesAb(abS);
    const juce::dsp::ProcessContextReplacing<float> contextSines(sinesAb);
    sinesDelayLine.process(contextSines);
    
    // = Transients =
    juce::dsp::AudioBlock<float> transAb(abT);
    const juce::dsp::ProcessContextReplacing<float> contextTrans(transAb);
    transientsDelayLine.process(contextTrans);
    
    // = Noise =
    juce::dsp::AudioBlock<float> noiseAb(abN);
    const juce::dsp::ProcessContextReplacing<float> contextNoise(noiseAb);
    noiseDelayLine.process(contextNoise);

    
    // ===== S + T + N =====
    buffer.copyFrom(0, 0, abS, 0, 0, numSamples);
    buffer.addFrom(0, 0, abT, 0, 0, numSamples);
    buffer.addFrom(0, 0, abN, 0, 0, numSamples);
    
    buffer.copyFrom (1, 0, buffer.getReadPointer (0), buffer.getNumSamples());
    
    // ===== Plotting =====
    waveformBufferServiceS->insertBuffers(abS);
    waveformBufferServiceT->insertBuffers(abT);
    waveformBufferServiceN->insertBuffers(abN);
    waveformBufferServiceOut->insertBuffers(buffer);

    
//    spectrumBufferServiceS->insertBuffers(abS);
//    spectrumBufferServiceT->insertBuffers(abT);
//    spectrumBufferServiceN->insertBuffers(abN);
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
