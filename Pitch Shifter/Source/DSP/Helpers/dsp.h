#pragma once
#include <JuceHeader.h>
using Vec1D = std::vector<float>;

namespace dsp::helpers {
inline void deinterleaveRealFFT(Vec1D &dest, const Vec1D &src, int numRealSamples) {
    jassert(src.size() >= numRealSamples * 2);
    jassert(dest.size() >= numRealSamples);
    for (auto i = 0; i < numRealSamples; i++) {
        dest[i] = src[2 * i];
    }
};

inline void deinterleaveImagFFT(Vec1D &dest, const Vec1D &src, int numRealSamples) {
    jassert(src.size() >= numRealSamples * 2);
    jassert(dest.size() >= numRealSamples);
    for (auto i = 0; i < numRealSamples; i++) {
        dest[i] = src[2 * i + 1];
    }
};

inline void absInterleavedFFT(Vec1D &dest, const Vec1D &src, int numRealSamples) {
    jassert(src.size() >= numRealSamples * 2);
    jassert(dest.size() >= numRealSamples);
    for (auto i = 0; i < numRealSamples; i++) {
        dest[i] = std::hypot(src[2 * i], src[2 * i + 1]);
    }
};

inline void interleaveRealFFT(Vec1D &dest, const Vec1D &src, int numRealSamples) {
    jassert(dest.size() >= numRealSamples * 2);
    jassert(src.size() >= numRealSamples);
    for (auto i = 0; i < numRealSamples; i++) {
        dest[2 * i] = src[i];
    }
};

inline void interleaveImagFFT(Vec1D &dest, const Vec1D &src, int numRealSamples) {
    jassert(dest.size() >= numRealSamples * 2);
    jassert(src.size() >= numRealSamples);
    for (auto i = 0; i < numRealSamples; i++) {
        dest[2 * i + 1] = src[i];
    }
};

inline void interleaveFFT(Vec1D &dest, const Vec1D &srcReal, const Vec1D &srcImag, int numRealSamples) {
    jassert(dest.size() >= numRealSamples * 2);
    jassert(srcReal.size() >= numRealSamples);
    jassert(srcImag.size() >= numRealSamples);
    for (auto i = 0; i < numRealSamples; i++) {
        dest[2 * i] = srcReal[i];
        dest[2 * i + 1] = srcImag[i];
    }
};

inline void deinterleaveFFT(Vec1D &destReal, Vec1D &destImag, const Vec1D &src, int numRealSamples) {
    jassert(src.size() >= numRealSamples * 2);
    jassert(destReal.size() >= numRealSamples);
    jassert(destImag.size() >= numRealSamples);
    for (auto i = 0; i < numRealSamples; i++) {
        destReal[i] = src[2 * i];
        destImag[i] = src[2 * i + 1];
    }
};

inline void logMagnitudeSpectrum(Vec1D &x) {
    for (auto i = 0; i < x.size(); i++) {
        x[i] = 10 * log10(x[i]);
    }
};

inline void inverseLogMagnitudeSpectrum(Vec1D &x) {
    for (auto j = 0; j < x.size(); j++) {
        x[j] = std::powf(10.f, x[j] / 10.f);
    }
};
} // namespace dsp::helpers
