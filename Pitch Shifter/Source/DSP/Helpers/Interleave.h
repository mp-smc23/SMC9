#pragma once
#include <JuceHeader.h>
using Vec1D = std::vector<float>;

namespace dsp::helpers{
    inline void deinterleaveRealFFT(Vec1D &dest, const Vec1D &src, int numRealSamples){
        jassert(src.size() >= numRealSamples * 2);
        jassert(dest.size() >= numRealSamples);
        for(auto i = 0; i < numRealSamples; i++){
            dest[i] = src[2 * i];
        }
    };
    
    inline void deinterleaveImagFFT(Vec1D &dest, const Vec1D &src, int numRealSamples){
        jassert(src.size() >= numRealSamples * 2);
        jassert(dest.size() >= numRealSamples);
        for(auto i = 0; i < numRealSamples; i++){
            dest[i] = src[2 * i + 1];
        }
    };
    
    inline void absInterleavedFFT(Vec1D &dest, const Vec1D &src, int numRealSamples){
        jassert(src.size() >= numRealSamples * 2);
        jassert(dest.size() >= numRealSamples);
        for(auto i = 0; i < numRealSamples; i++){
            dest[i] = std::hypot(src[2 * i], src[2 * i + 1]);
        }
    };

    inline void interleaveRealFFT(Vec1D &dest, const Vec1D &src, int numRealSamples){
        jassert(dest.size() >= numRealSamples * 2);
        jassert(src.size() >= numRealSamples);
        for(auto i = 0; i < numRealSamples; i++){
            dest[2*i] = src[i];
        }
    };
    
    inline void interleaveImagFFT(Vec1D &dest, const Vec1D &src, int numRealSamples){
        jassert(dest.size() >= numRealSamples * 2);
        jassert(src.size() >= numRealSamples);
        for(auto i = 0; i < numRealSamples; i++){
            dest[2*i + 1] = src[i];
        }
    };

}



