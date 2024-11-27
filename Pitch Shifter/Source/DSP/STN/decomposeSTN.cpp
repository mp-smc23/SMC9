#include "decomposeSTN.h"

dsp::DecomposeSTN::DecomposeSTN(
    std::shared_ptr<juce::dsp::ProcessSpec> procSpec)
    : processSpec(procSpec), forwardFFT(13), inverseFFTS(13), inverseFFTTN(13), forwardFFTTN(9),
      inverseFFTT(9), inverseFFTN(9){};

void dsp::DecomposeSTN::setWindowS(const int newWindowSizeS) {
    // Assert power of two
    const auto winSize = newWindowSizeS - (newWindowSizeS % 2);
    if(fftSizeS == winSize) return;
    
    DBG("New Window Size S = " + juce::String(winSize));
    fftSizeS = winSize;

    prepare();
}

void dsp::DecomposeSTN::setWindowTN(const int newWindowSizeTN) {
    // Assert power of two
    const auto winSize = newWindowSizeTN - (newWindowSizeTN % 2);
    if(fftSizeTN == winSize) return;
    
    DBG("New Window Size TN = " + juce::String(winSize));
    fftSizeTN = winSize;

    prepare();
}

void dsp::DecomposeSTN::setThresholdSines(const float thresholdLow) {
    threshold_s_2 = thresholdLow;
    threshold_s_1 = thresholdLow + 0.1f;
}

void dsp::DecomposeSTN::setThresholdTransients(const float thresholdLow) {
    threshold_tn_2 = thresholdLow;
    threshold_tn_1 = thresholdLow + 0.1f;
}

void dsp::DecomposeSTN::fuzzySTN(STN& stn, Vec1D& rt,
                                 const float G1, const float G2,
                                 medianfilter::HorizontalMedianFilter& filterH,
                                 medianfilter::VerticalMedianFilter& filterV) {

    const auto len = rt.size();
    
    const auto& xHorizonal = filterH.process(rt);
    const auto& xVertical = filterV.process(rt);
    transientness(rt, xHorizonal, xVertical);

    const auto tmp = juce::MathConstants<float>::pi / (2 * (G1 - G2));
    for (auto i = 0; i < len; i++) {
        const auto rs = 1 - rt[i];

        if (rs >= G1) {
            stn.S[i] = 1;
        } else if (rs >= G2) {
            const auto sinRs = std::sinf(tmp * (rs - G2));
            stn.S[i] = sinRs * sinRs;
        }
        else {
            stn.S[i] = 0.f;
        }

        if (rt[i] >= G1) {
            stn.T[i] = 1;
        } else if (rt[i] >= G2) {
            const auto sinRt = std::sinf(tmp * (rt[i] - G2));
            stn.T[i] = sinRt * sinRt;
        }
        else {
            stn.T[i] = 0.f;
        }

        stn.N[i] = 1 - stn.S[i] - stn.T[i];
    }
}

void dsp::DecomposeSTN::transientness(Vec1D& dest, const Vec1D &xHorizontal, const Vec1D &xVertical) {
    jassert(xHorizontal.size() == xVertical.size());
    jassert(xHorizontal.size() == dest.size());
    const auto len = dest.size();

    // Perform element-wise division
    for (auto i = 0; i < len; i++) {
        dest[i] = xVertical[i] / (xVertical[i] + xHorizontal[i] + std::numeric_limits<float>::epsilon());
    }
}

/// Only mono processing
void dsp::DecomposeSTN::process(const juce::AudioBuffer<float> &buffer,
                                juce::AudioBuffer<float> &S,
                                juce::AudioBuffer<float> &T,
                                juce::AudioBuffer<float> &N) {
    // TODO STEREO - later
    const auto numSamples = buffer.getNumSamples();
    
    const auto data = buffer.getReadPointer(0);
    
    // Fill S, T, N
    auto dataS = S.getWritePointer(0);
    auto dataT = T.getWritePointer(0);
    auto dataN = N.getWritePointer(0);
    
    for(auto i = 0; i < numSamples; i++){
        bufferInput[writePtr] = data[i];
        bufferTNSmall[writePtr2++] = bufferTN[writePtr];
        bufferTN[writePtr] = 0;
        
        writePtr++;
        if(writePtr >= bufferInput.size()) writePtr = 0; // circular buffer
        if(writePtr2 >= bufferTNSmall.size()) writePtr2 = 0; // circular buffer
        
        dataS[i] = bufferS[readPtr];
        dataT[i] = bufferT[readPtr];
        dataN[i] = bufferN[readPtr];
        bufferS[readPtr] = bufferT[readPtr] = bufferN[readPtr] = 0.f;
        readPtr++;
        
        if(readPtr >= bufferS.size()) readPtr = 0;
        
        newSamplesCount++;
        if(newSamplesCount >= hopSizeS){
            decompose_1(writePtr);
            newSamplesCount = 0;
        }
        
        newSamplesCount2++;
        if(newSamplesCount2 >= hopSizeTN){
            decompose_2(writePtr2);
            newSamplesCount2 = 0;
        }
         
    }
}

void dsp::DecomposeSTN::decompose_1(const int ptr){
    // Copy new samples to FFT vector
    juce::FloatVectorOperations::copy(fft_1.data(), bufferInput.data() + ptr, fftSizeS - ptr);
    juce::FloatVectorOperations::copy(fft_1.data() + (fftSizeS - ptr), bufferInput.data(), ptr);
    
    // Round 1
    juce::FloatVectorOperations::multiply(fft_1.data(), windowS.data(), fftSizeS); // windowing
    forwardFFT.performRealOnlyForwardTransform(fft_1.data()); // FFT
    
    helpers::absInterleavedFFT(rtS, fft_1, fftSizeS); // abs of complex vector
    // deinterleaving for easiness of calcs
    helpers::deinterleaveFFT(real_fft_1_s, imag_fft_1_s, fft_1, fftSizeS);
    juce::FloatVectorOperations::copy(real_fft_1_tn.data(), real_fft_1_s.data(), fftSizeS);
    juce::FloatVectorOperations::copy(imag_fft_1_tn.data(), imag_fft_1_s.data(), fftSizeS);
    
    fuzzySTN(stn1, rtS,
             threshold_s_1, threshold_s_2,
             medianFilterHorS, medianFilterVerS);
    
    juce::FloatVectorOperations::multiply(real_fft_1_s.data(), stn1.S.data(),  fftSizeS); // Apply sines mask real
    juce::FloatVectorOperations::multiply(imag_fft_1_s.data(), stn1.S.data(),  fftSizeS); // Apply sines mask imag
    
    juce::FloatVectorOperations::add(stn1.T.data(), stn1.N.data(), fftSizeS); // Add transients and noise masks
    juce::FloatVectorOperations::multiply(real_fft_1_tn.data(), stn1.T.data(), fftSizeS); // Apply summed mask real
    juce::FloatVectorOperations::multiply(imag_fft_1_tn.data(), stn1.T.data(), fftSizeS); // Apply summed mask imag
    
    // interleave the samples back
    helpers::interleaveFFT(fft_1, real_fft_1_s, imag_fft_1_s, fftSizeS);
    helpers::interleaveFFT(fft_1_tn, real_fft_1_tn, imag_fft_1_tn, fftSizeS);
    
    inverseFFTS.performRealOnlyInverseTransform(fft_1.data()); // IFFT
    inverseFFTTN.performRealOnlyInverseTransform(fft_1_tn.data());

    juce::FloatVectorOperations::multiply(fft_1.data(), windowS.data(), fftSizeS); // windowing
    juce::FloatVectorOperations::multiply(fft_1_tn.data(), windowS.data(), fftSizeS); // windowing
    
    juce::FloatVectorOperations::multiply(fft_1.data(), windowCorrection, fftSizeS); // overlap add scaling
    
    juce::FloatVectorOperations::multiply(fft_1_tn.data(), windowCorrection, fftSizeS); // overlap add scaling
    
    for (auto j = 0; j < fftSizeS; j++) {
        bufferS[(readPtr + j) % bufferS.size()] += fft_1[j];
        bufferTN[(ptr + j) % bufferTN.size()] += fft_1_tn[j];
    }
}

void dsp::DecomposeSTN::decompose_2(const int ptr){
    
    // Round 2
    // Copy new samples to FFT vector
    juce::FloatVectorOperations::copy(fft_2.data(), bufferTNSmall.data() + ptr, fftSizeTN - ptr);
    juce::FloatVectorOperations::copy(fft_2.data() + (fftSizeTN - ptr), bufferTNSmall.data(), ptr);

    juce::FloatVectorOperations::multiply(fft_2.data(), windowTN.data(), fftSizeTN); // windowing
    forwardFFTTN.performRealOnlyForwardTransform(fft_2.data());
    
    helpers::absInterleavedFFT(rtTN, fft_2, fftSizeTN); // abs of complex vector
    // deinterleaving for easiness of calcs
    helpers::deinterleaveFFT(real_fft_2_t, imag_fft_2_t, fft_2, fftSizeTN);
    juce::FloatVectorOperations::copy(real_fft_2_ns.data(), real_fft_2_t.data(), fftSizeTN);
    juce::FloatVectorOperations::copy(imag_fft_2_ns.data(), imag_fft_2_t.data(), fftSizeTN);
    
    
    fuzzySTN(stn2, rtTN,
             threshold_tn_1, threshold_tn_2,
             medianFilterHorTN, medianFilterVerTN);

    juce::FloatVectorOperations::multiply(real_fft_2_t.data(), stn2.T.data(), fftSizeTN); // Apply sines mask
    juce::FloatVectorOperations::multiply(imag_fft_2_t.data(), stn2.T.data(), fftSizeTN); // Apply sines mask

    juce::FloatVectorOperations::add(stn2.N.data(), stn2.S.data(), fftSizeTN); // Add noise and sines masks
    juce::FloatVectorOperations::multiply(real_fft_2_ns.data(), stn2.N.data(), fftSizeTN); // Apply summed mask
    juce::FloatVectorOperations::multiply(imag_fft_2_ns.data(), stn2.N.data(), fftSizeTN); // Apply summed mask
    
    // interleave the samples back
    helpers::interleaveFFT(fft_2, real_fft_2_t, imag_fft_2_t, fftSizeTN);
    helpers::interleaveFFT(fft_2_ns, real_fft_2_ns, imag_fft_2_ns, fftSizeTN);
    
    inverseFFTT.performRealOnlyInverseTransform(fft_2.data()); // IFFT
    inverseFFTN.performRealOnlyInverseTransform(fft_2_ns.data());

    juce::FloatVectorOperations::multiply(fft_2.data(), windowTN.data(), fftSizeTN); // windowing
    juce::FloatVectorOperations::multiply(fft_2_ns.data(), windowTN.data(), fftSizeTN); // windowing
    
    juce::FloatVectorOperations::multiply(fft_2.data(), windowCorrection, fftSizeTN); // overlap add scaling
    juce::FloatVectorOperations::multiply(fft_2_ns.data(), windowCorrection, fftSizeTN); // overlap add scaling

    for (auto j = 0; j < fftSizeTN; j++) {
        bufferT[(readPtr + j) % bufferT.size()] += fft_2[j];
        bufferN[(readPtr + j) % bufferN.size()] += fft_2_ns[j];
    }

}

void dsp::DecomposeSTN::prepare() {
    bufferInput.resize(fftSizeS);
    bufferS.resize(fftSizeS + fftSizeTN);
    bufferT.resize(fftSizeS + fftSizeTN);
    bufferN.resize(fftSizeS + fftSizeTN);
    bufferTN.resize(fftSizeS);
    bufferTNSmall.resize(fftSizeTN);
    
    const auto pow2S = log2(fftSizeS);
    forwardFFT = juce::dsp::FFT(pow2S);
    inverseFFTS = juce::dsp::FFT(pow2S);
    inverseFFTTN = juce::dsp::FFT(pow2S);

    hopSizeS = fftSizeS / overlap;
    windowS.resize(fftSizeS);
    fft_1.resize(fftSizeS*2);
    fft_1_tn.resize(fftSizeS*2);
    real_fft_1_s.resize(fftSizeS);
    imag_fft_1_s.resize(fftSizeS);
    real_fft_1_tn.resize(fftSizeS);
    imag_fft_1_tn.resize(fftSizeS);
    rtS.resize(fftSizeS);
    stn1.resize(fftSizeS);
    
    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        windowS.data(), fftSizeS,
        juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false);

    medianFilterHorS.setFilterSize(std::div(static_cast<int>(filterLengthTime * processSpec->sampleRate), hopSizeS).quot);
    medianFilterHorS.setSamplesSize(fftSizeS);

    medianFilterVerS.setFilterSize(std::div(static_cast<int>(filterLengthFreq * fftSizeS), static_cast<int>(processSpec->sampleRate)).quot);
    medianFilterVerS.setSamplesSize(fftSizeS);
    
    const auto pow2TN = log2(fftSizeTN);
    forwardFFTTN = juce::dsp::FFT(pow2TN);
    inverseFFTT = juce::dsp::FFT(pow2TN);
    inverseFFTN = juce::dsp::FFT(pow2TN);

    hopSizeTN = fftSizeTN / overlap;
    windowTN.resize(fftSizeTN);
    fft_2.resize(fftSizeTN * 2);
    fft_2_ns.resize(fftSizeTN * 2);
    real_fft_2_t.resize(fftSizeTN);
    imag_fft_2_t.resize(fftSizeTN);
    real_fft_2_ns.resize(fftSizeTN);
    imag_fft_2_ns.resize(fftSizeTN);
    rtTN.resize(fftSizeTN);
    stn2.resize(fftSizeTN);

    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        windowTN.data(), fftSizeTN,
        juce::dsp::WindowingFunction<float>::WindowingMethod::hann, false);
    
    medianFilterHorTN.setFilterSize(std::div(static_cast<int>(filterLengthTime * processSpec->sampleRate), hopSizeTN).quot);
    medianFilterHorTN.setSamplesSize(fftSizeTN);
    
    medianFilterVerTN.setFilterSize(std::div(static_cast<int>(filterLengthFreq * fftSizeTN), static_cast<int>(processSpec->sampleRate)).quot);
    medianFilterVerTN.setSamplesSize(fftSizeTN);

}
