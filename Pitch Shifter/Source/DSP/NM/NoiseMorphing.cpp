#include "NoiseMorphing.h"

dsp::NoiseMorphing::NoiseMorphing(std::shared_ptr<juce::dsp::ProcessSpec> procSpec)
    : forwardFFT{10}, forwardFFTNoise{10}, inverseFFT{10}, processSpec(procSpec){
        setPitchShiftSemitones(0);
        setFFTSize(2048);
    };

void dsp::NoiseMorphing::setPitchShiftSemitones(const int semitones) {
    pitchShiftRatio = std::powf(2.f, semitones / 12.f);
    hopSizeStretch = static_cast<int>(hopSize * pitchShiftRatio / 2.f);
    windowCorrectionStretch = (8.f * hopSizeStretch) / (3.f * fftSize);
    
    DBG("Pitch Shift Ratio = " + juce::String(pitchShiftRatio));
    DBG("Hop Size Stretch = " + juce::String(hopSizeStretch));
    DBG("Window Correction Stretch = " + juce::String(windowCorrectionStretch));
    
    jassert(hopSizeStretch * 2 == pitchShiftRatio * hopSize);
}

void dsp::NoiseMorphing::setFFTSize(const int newFFTSize) {
    // Assert power of two
    const auto fftSizePow2 = newFFTSize - (newFFTSize % 2);
    DBG("New Window Size = " + juce::String(fftSizePow2));
    fftSize = fftSizePow2;
    hopSize = fftSize / overlap;

    prepare();
}

void dsp::NoiseMorphing::process(juce::AudioBuffer<float> &buffer) {
    // TODO STEREO - later
    const auto numSamples = buffer.getNumSamples();
    const auto data = buffer.getWritePointer(0);

    for (auto i = 0; i < numSamples; i++) {
        input[writeReadPtrInput] = data[i];
        writeReadPtrInput++;
        if (writeReadPtrInput >= input.size())
            writeReadPtrInput = 0;
        
        data[i] = output[newSamplesCount];
        
        newSamplesCount++;
        if (newSamplesCount >= hopSize) {
            processFrame();
            newSamplesCount = 0;
            
            
            auto used = interpolator.process(pitchShiftRatio, stretchedUnwinded.data(), output.data(), hopSize, hopSizeStretch * 2, 0);
            
            if(used != hopSize * pitchShiftRatio){
                DBG(" ======== alert! ======== ");
                DBG("used = " + juce::String(used));
                DBG("hopSize * pitchShiftRatio = " + juce::String(hopSize * pitchShiftRatio));
            }
        }
    }
}

void dsp::NoiseMorphing::processFrame() {
    // Copy new samples to FFT vector
    juce::FloatVectorOperations::copy(fft.data(), input.data() + writeReadPtrInput, fftSize - writeReadPtrInput);
    juce::FloatVectorOperations::copy(fft.data() + (fftSize - writeReadPtrInput), input.data(), writeReadPtrInput);

    juce::FloatVectorOperations::multiply(fft.data(), window.data(), fftSize); // windowing
    forwardFFT.performRealOnlyForwardTransform(fft.data());                    // FFT

    helpers::absInterleavedFFT(fftAbs, fft, fftSize); // get abs value of noise

    // log-magnitude spectrum
    for (auto i = 0; i < fftAbs.size(); i++) {
        fftAbs[i] = 10 * log10(fftAbs[i]);
    }

    framesInterpolation();

    juce::FloatVectorOperations::copy(fftAbsPrev.data(), fftAbs.data(), fftSize);
    for(auto i = 1; i < interpolatedFrames.size(); i++){
        auto& frame = interpolatedFrames[i];
        for (auto j = 0; j < fftSize; j++) {
            frame[j] = std::powf(10.f, frame[j] / 10.f);
        }
        
        // Copy frame to the fft vector, both real and imag are filled with abs spectrum.
        // This will be used in noise morphing to run element-wise multiplication of abs(X)*E
        // which, for each element, expands to (X.Real * E.Real + i * X.Real * E.Imag)
        helpers::interleaveRealFFT(fft, frame, fftSize);
        helpers::interleaveImagFFT(fft, frame, fftSize);
        
        generateNoise(hopSizeStretch);
        
        noiseMorphing(fft, writeReadPtrNoise);

        inverseFFT.performRealOnlyInverseTransform(fft.data());
        
        juce::FloatVectorOperations::multiply(fft.data(), window.data(), fftSize); // windowing
        juce::FloatVectorOperations::multiply(fft.data(), windowCorrectionStretch, fftSize); // overlap add scaling
        
        for (auto j = 0; j < fftSize; j++) {
            stretched[(writePtrStretched + j) % stretched.size()] += fft[j];
        }
        writePtrStretched += hopSizeStretch;
        writePtrStretched %= stretched.size();
    }
    
    // copy contents of streched buffer to undwinded buffer - for easiness of resampling
    const auto copyStep1 = stretched.size() - readPtrStretched;
    juce::FloatVectorOperations::copy(stretchedUnwinded.data(), stretched.data() + readPtrStretched, copyStep1);
    juce::FloatVectorOperations::copy(stretchedUnwinded.data() + copyStep1, stretched.data(), readPtrStretched);

    // clear used stretched samples (hopSizeStretch * 2)
    for(auto i = 0; i < hopSizeStretch * 2; i++){
        stretched[readPtrStretched++] = 0;
        if(readPtrStretched >= stretched.size()) readPtrStretched = 0;
    }
}

// TODO generalize for higher factors
void dsp::NoiseMorphing::framesInterpolation() {
    juce::FloatVectorOperations::copy(interpolatedFrames[0].data(), fftAbsPrev.data(), fftSize);
    juce::FloatVectorOperations::copy(interpolatedFrames[2].data(), fftAbs.data(), fftSize);
    
    float nextIdx, prevNextDist, prevDist, nextDist;
    nextIdx = prevNextDist = pitchShiftRatio;
    prevDist = nextDist = prevNextDist / 2.f;
    
    auto& newFrame = interpolatedFrames[1];
    for (auto i = 0; i < fftSize; i++) {
        newFrame[i] = fftAbsPrev[i] * (nextDist / prevNextDist) + fftAbs[i] * (prevDist / prevNextDist);
    }
}

void dsp::NoiseMorphing::generateNoise(int size) {
    // generate as much noise as we need for morphing
    auto maxNoise = 0.f;
    auto startPtr = writeReadPtrNoise;
    for(auto j = 0; j < size; j++){
        whiteNoise[writeReadPtrNoise] = dist(generator);
        if(auto absNoise = std::abs(whiteNoise[writeReadPtrNoise]); absNoise > maxNoise){
            maxNoise = absNoise;
        }
        writeReadPtrNoise++;
        if(writeReadPtrNoise >= whiteNoise.size()) writeReadPtrNoise = 0;
    }
    
    for(auto j = 0; j < size; j++){
        whiteNoise[startPtr++] /= maxNoise;
        if(startPtr >= whiteNoise.size()) startPtr = 0;
    }
}

void dsp::NoiseMorphing::noiseMorphing(Vec1D &dest, const int ptr) {

    juce::FloatVectorOperations::copy(fftNoise.data(), whiteNoise.data() + ptr, fftSize - ptr);
    juce::FloatVectorOperations::copy(fftNoise.data() + (fftSize - ptr), whiteNoise.data(), ptr);

    juce::FloatVectorOperations::multiply(fftNoise.data(), window.data(), fftSize); // windowing

    forwardFFTNoise.performRealOnlyForwardTransform(fftNoise.data()); // FFT
    
    // normalize by the window energy to ensure spectral magnitude equals 1
    juce::FloatVectorOperations::multiply(fftNoise.data(), 1 / windowEnergy, fftSize * 2);
    
    // multiply each frame of the white noise by the interpolated frame of the input's noise (dest)
    juce::FloatVectorOperations::multiply(dest.data(), fftNoise.data(), fftSize * 2); // element wise multiplication
}

void dsp::NoiseMorphing::prepare() {
    input.resize(fftSize);
    whiteNoise.resize(fftSize);
    output.resize(hopSize);

    fft.resize(fftSize * 2);
    fftNoise.resize(fftSize * 2);

    fftAbs.resize(fftSize);
    fftAbsPrev.resize(fftSize);

    stretched.resize(fftSize * maxPitchShiftRatio);
    stretchedUnwinded.resize(fftSize * maxPitchShiftRatio);
    juce::FloatVectorOperations::fill(stretched.data(), 0.f, stretched.size());
    
    interpolatedFrames.resize(3);
    for (auto &e : interpolatedFrames) {
        e.resize(fftSize);
    }
    
    window.resize(fftSize);
    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        window.data(), fftSize, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false);

    auto sumWindowSq = 0.f;
    for (const auto &e : window) {
        sumWindowSq += e * e;
    }
    
    const auto pow2S = log2(fftSize);
    forwardFFT = juce::dsp::FFT(pow2S);
    inverseFFT = juce::dsp::FFT(pow2S);
    forwardFFTNoise = juce::dsp::FFT(pow2S);

    windowEnergy = std::sqrt(sumWindowSq);
    
    interpolator.reset();
    DBG("getBaseLatency: " + juce::String(interpolator.getBaseLatency()));
}
