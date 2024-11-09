#pragma once
#include <JuceHeader.h>

using Vec2D = std::vector<std::vector<float>>;
using Vec2DComplex = std::vector<std::vector<std::complex<float>>>;
using Vec1D = std::vector<float>;

namespace dsp {
class DecomposeSTN {
  public:
    DecomposeSTN(std::shared_ptr<juce::dsp::ProcessSpec> procSpec);
    ~DecomposeSTN() = default;

    void setWindowS(const int newWindowSizeS);
    void setWindowTN(const int newWindowSizeTN);

    void process(juce::AudioBuffer<float> &buffer) const;
    void prepare();

  private:
    std::tuple<Vec1D, Vec1D, Vec1D> fuzzySTN(const Vec1D xReal,
                                             const float threshold1,
                                             const float threshold2,
                                             const int medFilterHorizontalSize,
                                             const int medFilterVerticalSize);

    Vec1D filterHorizonal(const Vec1D x, const int filterSize);
    Vec1D filterVertical(const Vec1D x, const int filterSize);
    Vec1D transientness(const Vec1D& xHorizontal, const Vec1D& xVertical);
    
    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;

    juce::dsp::FFT forwardFFT;   // forward S+T+N
    juce::dsp::FFT inverseFFTS;  // inverse S
    juce::dsp::FFT forwardFFTTN; // forward T+N
    juce::dsp::FFT inverseFFTT;  // inverse T
    juce::dsp::FFT inverseFFTN;  // inverse N

    std::vector<float> windowS;
    std::vector<float> windowTN;

    int windowSizeS{1024};
    int windowSizeTN{1024};

    int hopSizeS{windowSizeS / 8};
    int hopSizeTN{windowSizeTN / 8};

    float filterLengthTime{0.2f};  // 200ms
    float filterLengthFreq{500.f}; // 500Hz
};
} // namespace dsp
