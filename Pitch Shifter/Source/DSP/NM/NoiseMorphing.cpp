#include "NoiseMorphing.h"

dsp::NoiseMorphing::NoiseMorphing(std::shared_ptr<juce::dsp::ProcessSpec> procSpec)
    : forwardFFT{10}, forwardFFTNoise{10}, inverseFFT{10}, processSpec(procSpec) {
    setPitchShiftSemitones(0);
    setFFTSize(2048);
};

void dsp::NoiseMorphing::setPitchShiftSemitones(const int semitones) {
    const auto ratio = std::powf(2.f, semitones / 12.f);
    this->setPitchShiftRatio(ratio);
}

void dsp::NoiseMorphing::setPitchShiftRatio(const float newPitchShiftRatio) {
    if (pitchShiftRatio == newPitchShiftRatio)
        return;

    pitchShiftRatio = newPitchShiftRatio;
    spectrumInterpolationFrames = std::ceil(pitchShiftRatio);

    hopSizeStretch = static_cast<int>(hopSize * pitchShiftRatio / static_cast<float>(spectrumInterpolationFrames));
    windowCorrectionStretch = 0.5 * fftSize / hopSizeStretch;

    DBG("Pitch Shift Ratio = " + juce::String(pitchShiftRatio));
    DBG("Hop Size Stretch = " + juce::String(hopSizeStretch));
    DBG("Window Correction Stretch = " + juce::String(windowCorrectionStretch));
}

void dsp::NoiseMorphing::setFFTSize(const int newFFTSize) {
    // Assert power of two
    const auto fftSizePow2 = newFFTSize - (newFFTSize % 2);
    if (fftSize == fftSizePow2)
        return;

    DBG("New Window Size = " + juce::String(fftSizePow2));
    fftSize = fftSizePow2;
    hopSize = fftSize / overlap;

    prepare();
}

void dsp::NoiseMorphing::process(juce::AudioBuffer<float> &buffer) {
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
    helpers::logMagnitudeSpectrum(fftAbs);            // log-magnitude spectrum

    stretchSpectrum(interpolatedFrames, fftAbsPrev, fftAbs);

    juce::FloatVectorOperations::copy(fftAbsPrev.data(), fftAbs.data(),
                                      fftSize); // save current frame for next interpolation
    for (auto i = 0; i < spectrumInterpolationFrames; i++) {
        auto &frame = interpolatedFrames[i];
        helpers::inverseLogMagnitudeSpectrum(frame);

        generateNoise(hopSizeStretch);

        if (!juce::approximatelyEqual(pitchShiftRatio, 1.f)) { // if pitch shift is 0 we don't need to do noise morphing
            // Copy frame to the fft vector, both real and imag are filled with abs spectrum.
            // This will be used in noise morphing to run element-wise multiplication of abs(X)*E
            // which, for each element, expands to (X.Real * E.Real + i * X.Real * E.Imag)
            helpers::interleaveFFT(fft, frame, frame, fftSize);

            noiseMorphing(fft);
        }

        
        inverseFFT.performRealOnlyInverseTransform(fft.data()); // IFFT
        
        juce::FloatVectorOperations::multiply(fft.data(), 1.f / windowCorrectionStretch, fftSize); // overlap add scaling
        
        for (auto j = 0; j < fftSize; j++) {
            stretched[(writePtrStretched + j) % stretched.size()] += fft[j];
        }
        writePtrStretched += hopSizeStretch;
        writePtrStretched %= stretched.size();
    }

    // copy contents of streched buffer to undwinded buffer - for easiness of resampling
    const auto copyStepSamples = stretched.size() - readPtrStretched;
    juce::FloatVectorOperations::copy(stretchedUnwinded.data(), stretched.data() + readPtrStretched, copyStepSamples);
    juce::FloatVectorOperations::copy(stretchedUnwinded.data() + copyStepSamples, stretched.data(), readPtrStretched);

    // clear used stretched samples (hopSizeStretch * 2)
    for (auto i = 0; i < hopSizeStretch * spectrumInterpolationFrames; i++) {
        stretched[readPtrStretched++] = 0;
        if (readPtrStretched >= stretched.size())
            readPtrStretched = 0;
    }

    [[maybe_unused]] auto _ = interpolator.process(pitchShiftRatio, stretchedUnwinded.data(), output.data(), hopSize,
                                                   hopSizeStretch * spectrumInterpolationFrames, 0);
}

void dsp::NoiseMorphing::stretchSpectrum(Vec2D &dest, const Vec1D &frame1, const Vec1D &frame2) {
    const auto lastFrame = spectrumInterpolationFrames - 1;
    juce::FloatVectorOperations::copy(dest[lastFrame].data(), frame2.data(), fftSize); // copy frame2 to the last frame

    const auto frac = 1.f / static_cast<float>(spectrumInterpolationFrames);
    for (auto i = 0; i < spectrumInterpolationFrames; i++) {
        interpolateFrames(dest[i], frame1, frame2, frac + i * frac);
    }
}

void dsp::NoiseMorphing::interpolateFrames(Vec1D &dest, const Vec1D &frame1, const Vec1D &frame2, const float frac) {
    jassert(dest.size() == frame1.size());
    jassert(frame2.size() == frame1.size());
    const auto prevNextDist = 1.f;
    const auto prevDist = frac;
    const auto nextDist = 1.f - frac;

    for (auto i = 0; i < dest.size(); i++) {
        dest[i] = frame1[i] * (nextDist / prevNextDist) + frame2[i] * (prevDist / prevNextDist);
    }
}

void dsp::NoiseMorphing::generateNoise(int size) {
    // generate as much noise as we need for morphing
    const auto &noise = noiseGenerator.generateNormalizedNoise(size);

    const auto copyStepSamples = juce::jmin(static_cast<int>(whiteNoise.size()) - writeReadPtrNoise, size);
    juce::FloatVectorOperations::copy(whiteNoise.data() + writeReadPtrNoise, noise.data(), copyStepSamples);
    juce::FloatVectorOperations::copy(whiteNoise.data(), noise.data() + copyStepSamples, size - copyStepSamples);
}

void dsp::NoiseMorphing::noiseMorphing(Vec1D &dest) {

    juce::FloatVectorOperations::copy(fftNoise.data(), whiteNoise.data() + writeReadPtrNoise,
                                      fftSize - writeReadPtrNoise);
    juce::FloatVectorOperations::copy(fftNoise.data() + (fftSize - writeReadPtrNoise), whiteNoise.data(),
                                      writeReadPtrNoise);

    juce::FloatVectorOperations::multiply(fftNoise.data(), windowNoise.data(), fftSize); // windowing

    forwardFFTNoise.performRealOnlyForwardTransform(fftNoise.data()); // FFT

    // normalize by the frame energy to ensure spectral magnitude equals 1
    juce::FloatVectorOperations::multiply(fftNoise.data(), 100.f / windowEnergy, fftSize * 2);

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

    interpolatedFrames.resize(maxPitchShiftRatio);
    for (auto &e : interpolatedFrames) {
        e.resize(fftSize);
    }

    window.resize(fftSize + 1);
    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        window.data(), fftSize + 1, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false);

    windowNoise.resize(fftSize + 1);
    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        windowNoise.data(), fftSize + 1, juce::dsp::WindowingFunction<float>::WindowingMethod::hann, true);

    auto sumWindowSq = 0.f;
    for (const auto &e : window) {
        sumWindowSq += e * e;
    }
    windowEnergy = sumWindowSq;
    
    const auto pow2S = log2(fftSize);
    forwardFFT = juce::dsp::FFT(pow2S);
    inverseFFT = juce::dsp::FFT(pow2S);
    forwardFFTNoise = juce::dsp::FFT(pow2S);

    noiseGenerator.prepare(hopSize * maxPitchShiftRatio);
    interpolator.reset();
    DBG("getBaseLatency: " + juce::String(interpolator.getBaseLatency()));
}
