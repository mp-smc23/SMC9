#include "NoiseMorphing.h"

dsp::NoiseMorphing::NoiseMorphing(std::shared_ptr<juce::dsp::ProcessSpec> procSpec)
    : forwardFFT{10}, forwardFFTNoise{10}, inverseFFT{10}, processSpec(procSpec){};

void dsp::NoiseMorphing::setPitchShiftSemitones(const int semitones) {
    pitchShiftRatio = static_cast<float>(semitones * semitones) / 144.f;
}

void dsp::NoiseMorphing::setFFTSize(const int newFFTSize) {
    // Assert power of two
    const auto fftSizePow2 = newFFTSize - (newFFTSize % 2);
    DBG("New Window Size S = " + juce::String(fftSizePow2));
    fftSize = fftSizePow2;
    hopSize = fftSize / overlap;

    prepare();
}

void dsp::NoiseMorphing::process(juce::AudioBuffer<float> &buffer) {
    // TODO STEREO - later
    const auto numSamples = buffer.getNumSamples();
    const auto data = buffer.getWritePointer(0);

    for (auto i = 0; i < numSamples; i++) {
        input[writePtr] = data[i];
        writePtr++;
        if (writePtr >= input.size())
            writePtr = 0;

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

    helpers::absInterleavedFFT(fftAbs, fft, fftSize); // get abs value of noise

    // log-magnitude spectrum
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

    // TODO generate as much noise as we need
    whiteNoise[writePtr] = dist(generator);
    
    // Copy frame to the fft vector, both real and imag are filled with abs spectrum.
    // This will be used in noise morphing to run element-wise multiplication of abs(X)*E
    // which, for each element, expands to (X.Real * E.Real + i * X.Real * E.Imag)
    helpers::interleaveRealFFT(fft, fftAbs, fftSize);
    helpers::interleaveImagFFT(fft, fftAbs, fftSize);
    noiseMorphing(fft, writePtr);

    inverseFFT.performRealOnlyInverseTransform(fft.data());
    
    juce::FloatVectorOperations::multiply(fft.data(), window.data(), fftSize);
    
    // }
    
    
    interpolator.process(pitchShiftRatio, stretched.data(), fft.data(), fftSize);
    
    for (auto j = 0; j < fftSize; j++) {
        bufferOutput[(readPtr + j) % bufferOutput.size()] += fft[j]; // TODO podmienic fft na cokolwiek co bedzie wynikowe
    }
}

void dsp::NoiseMorphing::framesInterpolation() {
    // TODO figure out what the heck to do with this
    Vec1D newFrame(fftSize);
    auto nextIdx = pitchShiftRatio;
    const auto prevNextDist = nextIdx;
    const auto prevDist = 1;
    const auto nextDist = nextIdx - 1;
    for (auto i = 0; i < fftSize; i++) {
        newFrame[i] = fftAbsPrev[i] * (nextDist / prevNextDist) + fftAbs[i] * (prevDist / prevNextDist);
    }
}

void dsp::NoiseMorphing::noiseMorphing(Vec1D &dest, const int ptr) {

    juce::FloatVectorOperations::copy(fftNoise.data(), whiteNoise.data() + ptr, fftSize - ptr);
    juce::FloatVectorOperations::copy(fftNoise.data() + (fftSize - ptr), whiteNoise.data(), ptr);

    juce::FloatVectorOperations::multiply(fftNoise.data(), window.data(), fftSize); // windowing

    forwardFFTNoise.performRealOnlyForwardTransform(fftNoise.data()); // FFT
    
    // normalize by the window energy to ensure spectral magnitude equals 1
    juce::FloatVectorOperations::multiply(fftNoise.data(), 1.f / windowEnergy, fftSize * 2);
    
    // multiply each frame of the white noise by the interpolated frame of the input's noise (dest)
    juce::FloatVectorOperations::multiply(dest.data(), fftNoise.data(), fftSize * 2); // element wise multiplication
}

void dsp::NoiseMorphing::prepare() {
    input.resize(fftSize);
    whiteNoise.resize(fftSize);
    bufferOutput.resize(fftSize);

    fft.resize(fftSize * 2);
    fftNoise.resize(fftSize * 2);

    fftAbs.resize(fftSize);
    fftAbsPrev.resize(fftSize);

    stretched.resize(fftSize * maxPitchShiftRatio);

    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        window.data(), fftSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false);

    auto sumWindowSq = 0.f;
    for (const auto &e : window) {
        sumWindowSq += e * e;
    }

    windowEnergy = std::sqrt(sumWindowSq);
    
    interpolator.reset();
    DBG("getBaseLatency: " + juce::String(interpolator.getBaseLatency()));
}
