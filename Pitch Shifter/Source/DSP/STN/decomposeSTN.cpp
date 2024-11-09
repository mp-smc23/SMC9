#include "decomposeSTN.h"

dsp::DecomposeSTN::DecomposeSTN(
    std::shared_ptr<juce::dsp::ProcessSpec> procSpec)
    : processSpec(procSpec), forwardFFT(13), inverseFFTS(13), forwardFFTTN(13),
      inverseFFTT(13){};

void dsp::DecomposeSTN::setWindowS(const int newWindowSizeS) {
    // Assert power of two
    const auto winSize = newWindowSizeS - (newWindowSizeS % 2);
    const auto pow2 = log2(winSize);

    forwardFFTTN = juce::dsp::FFT(pow2);
    inverseFFTT = juce::dsp::FFT(pow2);

    windowSizeS = winSize;
    hopSizeS = windowSizeS / 8;
    windowS.resize(windowSizeS);

    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        windowS.data(), newWindowSizeS,
        juce::dsp::WindowingFunction<float>::WindowingMethod::hann);
}

void dsp::DecomposeSTN::setWindowTN(const int newWindowSizeTN) {
    // Assert power of two
    const auto winSize = newWindowSizeTN - (newWindowSizeTN % 2);
    const auto pow2 = log2(winSize);

    forwardFFTTN = juce::dsp::FFT(pow2);
    inverseFFTT = juce::dsp::FFT(pow2);

    windowSizeTN = winSize;
    hopSizeTN = windowSizeTN / 8;
    windowTN.resize(windowSizeTN);

    juce::dsp::WindowingFunction<float>::fillWindowingTables(
        windowTN.data(), windowSizeTN,
        juce::dsp::WindowingFunction<float>::WindowingMethod::hann);
}

// TODO better types so we avoid useless allocation
std::tuple<Vec1D, Vec1D, Vec1D> dsp::DecomposeSTN::fuzzySTN(
    const Vec1D xReal, const float threshold1, const float threshold2,
    const int medFilterHorizontalSize, const int medFilterVerticalSize) {

    const auto len = xReal.size();
    const auto xHorizonal = filterHorizonal(xReal, medFilterHorizontalSize);
    const auto xVertical = filterVertical(xReal, medFilterVerticalSize);
    auto rt = transientness(xHorizonal, xVertical);

    Vec1D S(len, 0.f);
    Vec1D T(len, 0.f);
    Vec1D N(len, 0.f);

    const auto tmp =
        juce::MathConstants<float>::pi / (2 * (threshold1 - threshold2));
    for (auto i = 0; i < len; i++) {
        const auto rs = 1 - rt[i];

        if (rs >= threshold1) {
            S[i] = 1;
        } else if (rs >= threshold2) {
            const auto sinRs = std::sinf(tmp * (rs - threshold2));
            S[i] = sinRs * sinRs;
        }

        if (rt[i] >= threshold1) {
            T[i] = 1;
        } else if (rt[i] >= threshold2) {
            const auto sinRt = std::sinf(tmp * (rt[i] - threshold2));
            T[i] = sinRt * sinRt;
        }

        N[i] = 1 - S[i] - T[i];
    }

    return std::tuple<Vec1D, Vec1D, Vec1D>(S, T, N);
}

Vec1D transientness(const Vec1D &xHorizontal, const Vec1D &xVertical) {
    jassert(xHorizontal.size() == xVertical.size());
    const auto len = xHorizontal.size();

    Vec1D result(len);
    // Perform element-wise division
    for (auto i = 0; i < len; i++) {
        result[i] =
            xVertical[i] /
            (xVertical[i] + xHorizontal[i] +
             std::numeric_limits<float>::epsilon()); // TODO Perhaps if denom ==
                                                     // 0 then 0
    }

    return result;
}
