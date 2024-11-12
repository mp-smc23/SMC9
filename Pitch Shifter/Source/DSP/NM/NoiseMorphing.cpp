#include "NoiseMorphing.h"

dsp::NoiseMorphing::NoiseMorphing(std::shared_ptr<juce::dsp::ProcessSpec> procSpec)
    : forwardFFT{10}, forwardFFTNoise{10}, inverseFFT{10}, processSpec(procSpec){};

void dsp::NoiseMorphing::setPitchShiftSemitones(const int semitones) {
    pitchShiftRatio = static_cast<float>(semitones * semitones) / 144.f;
}

void dsp::NoiseMorphing::setFFTSize(const int newFFTSize) {
    fftSize = newFFTSize;
    hopSize = fftSize / overlap;

    prepare();
}

void dsp::NoiseMorphing::process(juce::AudioBuffer<float> &buffer) {
    // TODO STEREO - later
    const auto numSamples = buffer.getNumSamples();
    const auto data = buffer.getWritePointer(0);

    for (auto i = 0; i < numSamples; i++) {
        input[writePtr++] = data[i];
        if(writePtr >= input.size()) writePtr = 0;

        data[i] = bufferOutput[readPtr];
        bufferOutput[readPtr] = 0.f;
        if (++readPtr >= bufferOutput.size())
            readPtr = 0;

        newSamplesCount++;
        if (newSamplesCount >= hopSize) {
            processFrame();
            newSamplesCount = 0;
        }
    }
}

void dsp::NoiseMorphing::processFrame() {
    // Copy new samples to FFT vector
    juce::FloatVectorOperations::copy(fft.data(), input.data() + writePtr, fftSize - writePtr);
    juce::FloatVectorOperations::copy(fft.data() + (fftSize - writePtr), input.data(), writePtr);
    
    juce::FloatVectorOperations::multiply(fft.data(), window.data(), fftSize); // windowing
    forwardFFT.performRealOnlyForwardTransform(fft.data());                    // FFT

    helpers::deinterleaveRealFFT(fftAbs, fft, fftSize); // deinterleaving for easiness of calcs

    for (auto i = 0; i < fftSize; i++) {
        fftAbs[i] = 10 * log10(fabsf(fftAbs[i]));
    }

    framesInterpolation();

    juce::FloatVectorOperations::copy(fftAbsPrev.data(), fftAbs.data(), fft.size());

    // todo for dla każego zwróconego frame'a
    // for(auto i = 0; i < ???; i++){
    for (auto i = 0; i < fftSize; i++) {
        fftAbs[i] = std::powf(10.f, fftAbs[i] / 10.f);
    }
    
    Vec1D x;
    noiseMorphing(x);
    
    // }
}

void dsp::NoiseMorphing::framesInterpolation() {}

void dsp::NoiseMorphing::noiseMorphing(Vec1D &dest) {}

void dsp::NoiseMorphing::prepare() {}
