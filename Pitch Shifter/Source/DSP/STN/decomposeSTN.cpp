#include "decomposeSTN.h"

dsp::DecomposeSTN::DecomposeSTN(
    std::shared_ptr<juce::dsp::ProcessSpec> procSpec)
    : processSpec(procSpec), forwardFFT(13), inverseFFTS(13), inverseFFTTN(13), forwardFFTTN(9),
      inverseFFTT(9), inverseFFTN(9){};

void dsp::DecomposeSTN::setWindowS(const int newWindowSizeS) {
    // Assert power of two
    const auto winSize = newWindowSizeS - (newWindowSizeS % 2);
    DBG("New Window Size S = " + juce::String(winSize));
    windowSizeTN = winSize;

    prepare();
}

void dsp::DecomposeSTN::setWindowTN(const int newWindowSizeTN) {
    // Assert power of two
    const auto winSize = newWindowSizeTN - (newWindowSizeTN % 2);
    DBG("New Window Size TN = " + juce::String(winSize));
    windowSizeTN = winSize;

    prepare();
}

// TODO better types so we avoid useless allocation
void dsp::DecomposeSTN::fuzzySTN(Vec1D &S, Vec1D &T, Vec1D &N,
                                 const Vec1D& xReal, const float G1, const float G2,
                                 const int medFilterHorizontalSize,
                                 const int medFilterVerticalSize,
                                 std::deque<std::vector<float>>& filterHorizontal) {

    const auto len = xReal.size();
    const auto xHorizonal = filterHorizonal(xReal, filterHorizontal, medFilterHorizontalSize);
    const auto xVertical = filterVertical(xReal, medFilterVerticalSize);
    auto rt = transientness(xHorizonal, xVertical);

    const auto tmp = juce::MathConstants<float>::pi / (2 * (G1 - G2));
    for (auto i = 0; i < len; i++) {
        const auto rs = 1 - rt[i];

        if (rs >= G1) {
            S[i] = 1;
        } else if (rs >= G2) {
            const auto sinRs = std::sinf(tmp * (rs - G2));
            S[i] = sinRs * sinRs;
        }
        else {
            S[i] = 0.f;
        }

        if (rt[i] >= G1) {
            T[i] = 1;
        } else if (rt[i] >= G2) {
            const auto sinRt = std::sinf(tmp * (rt[i] - G2));
            T[i] = sinRt * sinRt;
        }
        else {
            T[i] = 0.f;
        }

        N[i] = 1 - S[i] - T[i];
    }
}

Vec1D dsp::DecomposeSTN::transientness(const Vec1D &xHorizontal, const Vec1D &xVertical) {
    jassert(xHorizontal.size() == xVertical.size());
    const auto len = xHorizontal.size();

    Vec1D result(len);
    // Perform element-wise division
    for (auto i = 0; i < len; i++) {
        result[i] = xVertical[i] / (xVertical[i] + xHorizontal[i] + std::numeric_limits<float>::epsilon());
        // TODO Perhaps if denom == 0 then 0
    }

    return result;
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
        bufferInput[writePtr++] = data[i];
        if(writePtr >= bufferInput.size()) writePtr = 0; // circular buffer
        
        dataS[i] = bufferS[readPtr];
        dataT[i] = bufferT[readPtr];
        dataN[i] = bufferN[readPtr];
        bufferS[readPtr] = bufferT[readPtr] = bufferN[readPtr] = bufferTN[readPtr] = 0.f;
        readPtr++;
        
        if(readPtr >= bufferS.size()) readPtr = 0;
        
        newSamplesCount++;
        if(newSamplesCount >= hopSizeS){
            decompose();
            newSamplesCount = 0;
        }
    }
}

void dsp::DecomposeSTN::decompose(){
    // Copy new samples to FFT vector
    juce::FloatVectorOperations::copy(fft_1.data(), bufferInput.data() + writePtr, windowSizeS - writePtr);
    juce::FloatVectorOperations::copy(fft_1.data() + (windowSizeS - writePtr), bufferInput.data(), writePtr);
    
    // Round 1
    juce::FloatVectorOperations::multiply(fft_1.data(), windowS.data(), windowSizeS); // windowing
    forwardFFT.performRealOnlyForwardTransform(fft_1.data()); // FFT
    
    deinterleaveRealFFT(real_fft_1, fft_1, windowSizeS); // deinterleaving for easiness of calcs
    
    fuzzySTN(S1, T1, N1, real_fft_1,
             threshold_s_1, threshold_s_2,
             medFilterHorizontalSizeS, medFilterVerticalSizeS,
             horizontalS);
    
    juce::FloatVectorOperations::multiply(S1.data(), real_fft_1.data(), windowSizeS); // Apply sines mask
    
    juce::FloatVectorOperations::add(T1.data(), N1.data(), windowSizeS); // Add transients and noise masks
    juce::FloatVectorOperations::multiply(T1.data(), real_fft_1.data(), windowSizeS); // Apply summed mask
    
    // interleave the samples back
    juce::FloatVectorOperations::copy(fft_1_tn.data(), fft_1.data(), fft_1.size());
    interleaveRealFFT(fft_1, S1, windowSizeS);
    interleaveRealFFT(fft_1_tn, T1, windowSizeS);
    
    inverseFFTS.performRealOnlyInverseTransform(fft_1.data()); // IFFT
    inverseFFTTN.performRealOnlyInverseTransform(fft_1_tn.data());

    juce::FloatVectorOperations::multiply(fft_1.data(), windowS.data(), windowSizeS); // windowing
    juce::FloatVectorOperations::multiply(fft_1_tn.data(), windowS.data(), windowSizeS); // windowing
    
    juce::FloatVectorOperations::multiply(fft_1.data(), 1.f/8.f, windowSizeS); // overlap add scaling
    juce::FloatVectorOperations::add(bufferS.data() + readPtr, fft_1.data(), windowSizeS - readPtr); // add samples to circular buffer
    juce::FloatVectorOperations::add(bufferS.data(), fft_1.data() + (windowSizeS - readPtr), readPtr); // add samples to circular buffer
    
    juce::FloatVectorOperations::multiply(fft_1_tn.data(), 1.f/8.f, windowSizeS); // overlap add scaling
    juce::FloatVectorOperations::add(bufferTN.data() + readPtr, fft_1_tn.data(), windowSizeS - readPtr); // add samples to circular buffer
    juce::FloatVectorOperations::add(bufferTN.data(), fft_1_tn.data() + (windowSizeS - readPtr), readPtr); // add samples to circular buffer
    
    // Round 2
    for(auto i = 0; i < windowSizeS / windowSizeTN; i++)
    {
        // Copy new samples to FFT vector
        const auto startIndex = (i * windowSizeTN + readPtr) % windowSizeS;
        for (auto j = 0; j < windowSizeTN; j++) {
            fft_2[j] = bufferTN[(startIndex + j) % windowSizeS];
        }
        
        juce::FloatVectorOperations::multiply(fft_2.data(), windowTN.data(), windowSizeTN); // windowing
        forwardFFTTN.performRealOnlyForwardTransform(fft_2.data());
        
        deinterleaveRealFFT(real_fft_2, fft_2, windowSizeTN); // deinterleaving for easiness of calcs
        
        fuzzySTN(S2, T2, N2, real_fft_2,
                 threshold_tn_1, threshold_tn_2,
                 medFilterHorizontalSizeTN, medFilterVerticalSizeTN,
                 horizontalTN);
        
        juce::FloatVectorOperations::multiply(T2.data(), real_fft_2.data(), windowSizeTN); // Apply sines mask
        
        juce::FloatVectorOperations::add(N2.data(), S2.data(), windowSizeTN); // Add noise and sines masks
        juce::FloatVectorOperations::multiply(N2.data(), real_fft_2.data(), windowSizeTN); // Apply summed mask
        
        // interleave the samples back
        juce::FloatVectorOperations::copy(fft_2_ns.data(), fft_2.data(), fft_2.size());
        interleaveRealFFT(fft_2, T2, windowSizeTN);
        interleaveRealFFT(fft_2_ns, N2, windowSizeTN);
        
        inverseFFTT.performRealOnlyInverseTransform(fft_2.data()); // IFFT
        inverseFFTN.performRealOnlyInverseTransform(fft_2_ns.data());

        juce::FloatVectorOperations::multiply(fft_2.data(), windowTN.data(), windowSizeTN); // windowing
        juce::FloatVectorOperations::multiply(fft_2_ns.data(), windowTN.data(), windowSizeTN); // windowing
        
        juce::FloatVectorOperations::multiply(fft_2.data(), 1.f/8.f, windowSizeTN); // overlap add scaling
        juce::FloatVectorOperations::multiply(fft_2_ns.data(), 1.f/8.f, windowSizeTN); // overlap add scaling
        
        for (auto j = 0; j < windowSizeTN; j++) {
            bufferT[(startIndex + j) % windowSizeS] += fft_2[j];
            bufferN[(startIndex + j) % windowSizeS] += fft_2_ns[j];
        }
    }
}


void dsp::DecomposeSTN::deinterleaveRealFFT(Vec1D &dest, const Vec1D &src, int numRealSamples){
    jassert(src.size() >= numRealSamples * 2);
    jassert(dest.size() >= numRealSamples);
    for(auto i = 0; i < numRealSamples; i++){
        dest[i] = src[2 * i];
    }
}

void dsp::DecomposeSTN::interleaveRealFFT(Vec1D &dest, const Vec1D &src, int numRealSamples){
    jassert(dest.size() >= numRealSamples * 2);
    jassert(src.size() >= numRealSamples);
    for(auto i = 0; i < numRealSamples; i++){
        dest[2*i] = src[i];
    }
}

Vec1D dsp::DecomposeSTN::filterHorizonal(const Vec1D& x, std::deque<std::vector<float>>& filter, const int filterSize){
    const auto numChannels = filter.size();
    const auto numSamples = x.size();
    Vec1D medianHorizontal(numSamples, 0.0f);

    const auto pad = filterSize / 2;
    Vec1D median(numChannels + 2 * pad, 0.0f);  // TODO allocate in adv
    Vec1D kernel(filterSize, 0.0f);  // TODO allocate in adv

    filter.pop_front();
    filter.push_back(x);
    
    for (int h = 0; h < numSamples; ++h)
        {
            for (int i = 0; i < numChannels; ++i)
            {
                median[i + pad] = filter[i][h]; // TODO use more optimal structure
            }
            // Extract the kernel directly from med_vec using indexing
            juce::FloatVectorOperations::copy(kernel.data(),
                                              median.data() + (numChannels - 1),
                                              filterSize);
            // Find the median element using nth_element
            std::nth_element(kernel.begin(), kernel.begin() + pad, kernel.end());
            medianHorizontal[h] = kernel[pad];
        }

    return medianHorizontal;
}

Vec1D dsp::DecomposeSTN::filterVertical(const Vec1D& x, const int filterSize){
    const auto numSamples = x.size();
    Vec1D medianVertical(numSamples, 0.0f); // TODO allocate in adv

    const auto pad = filterSize / 2;
    Vec1D median(numSamples + 2 * pad, 0.0f);  // TODO allocate in adv
    Vec1D kernel(filterSize, 0.0f);  // TODO allocate in adv

    for (auto h = 0; h < numSamples; h++)
    {
        median[h + pad] = x[h];

        juce::FloatVectorOperations::copy(kernel.data(),
                                          median.data() + h,
                                          filterSize);

        // Find the median element using nth_element
        std::nth_element(kernel.begin(), kernel.begin() + pad, kernel.end());
        medianVertical[h] = kernel[pad];
    }
    

    return medianVertical;
}

void dsp::DecomposeSTN::prepare() {
    // TODO let's see
    bufferInput.resize(windowSizeS);
    bufferS.resize(windowSizeS);
    bufferT.resize(windowSizeS);
    bufferN.resize(windowSizeS);
    bufferTN.resize(windowSizeS);

    const auto pow2S = log2(windowSizeS);
    forwardFFT = juce::dsp::FFT(pow2S);
    inverseFFTS = juce::dsp::FFT(pow2S);

    hopSizeS = windowSizeS / 8;
    windowS.resize(windowSizeS);
    fft_1.resize(windowSizeS*2);
    fft_1_tn.resize(windowSizeS*2);
    real_fft_1.resize(windowSizeS);
    S1.resize(windowSizeS);
    T1.resize(windowSizeS);
    N1.resize(windowSizeS);
    
    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        windowS.data(), windowSizeS,
        juce::dsp::WindowingFunction<float>::WindowingMethod::hann);

    medFilterHorizontalSizeS = std::div(static_cast<int>(filterLengthTime * processSpec->sampleRate), hopSizeS).quot;
    medFilterVerticalSizeS = std::div(static_cast<int>(filterLengthFreq * windowSizeS), static_cast<int>(processSpec->sampleRate)).quot;

    horizontalS.resize(medFilterHorizontalSizeS);
    for(auto& vec : horizontalS) vec.resize(windowSizeS);
    
    const auto pow2TN = log2(windowSizeTN);
    forwardFFTTN = juce::dsp::FFT(pow2TN);
    inverseFFTT = juce::dsp::FFT(pow2TN);
    inverseFFTN = juce::dsp::FFT(pow2TN);

    hopSizeTN = windowSizeTN / 8;
    windowTN.resize(windowSizeTN);
    fft_2.resize(windowSizeTN * 2);
    fft_2_ns.resize(windowSizeTN * 2);
    real_fft_2.resize(windowSizeTN);
    S2.resize(windowSizeTN);
    T2.resize(windowSizeTN);
    N2.resize(windowSizeTN);

    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        windowTN.data(), windowSizeTN,
        juce::dsp::WindowingFunction<float>::WindowingMethod::hann);
    
    medFilterHorizontalSizeTN = std::div(static_cast<int>(filterLengthTime * processSpec->sampleRate), hopSizeTN).quot;
    medFilterVerticalSizeTN = std::div(static_cast<int>(filterLengthFreq * windowSizeTN), static_cast<int>(processSpec->sampleRate)).quot;

    horizontalTN.resize(medFilterHorizontalSizeTN);
    for(auto& vec : horizontalTN) vec.resize(windowSizeTN);
}
