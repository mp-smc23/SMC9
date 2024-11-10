#pragma once
#include <JuceHeader.h>

using Vec2D = std::vector<std::vector<float>>;
using Vec1D = std::vector<float>;

namespace dsp {
class DecomposeSTN {
  public:
    DecomposeSTN(std::shared_ptr<juce::dsp::ProcessSpec> procSpec);
    ~DecomposeSTN() = default;

    void setWindowS(const int newWindowSizeS);
    void setWindowTN(const int newWindowSizeTN);

    void process(const juce::AudioBuffer<float> &buffer,
                 juce::AudioBuffer<float> &S, juce::AudioBuffer<float> &T,
                 juce::AudioBuffer<float> &N);
    void prepare();

  private:
    void decompose_1(const int ptr);
    void decompose_2(const int ptr);
    void fuzzySTN(Vec1D &S, Vec1D &T, Vec1D &N,
                  const Vec1D &xReal, const float threshold1,
                  const float threshold2, const int medFilterHorizontalSize,
                  const int medFilterVerticalSize,
                  Vec1D& filterV,
                  std::deque<std::vector<float>> &filterH);

    Vec1D filterHorizonal(const Vec1D &x,
                          std::deque<std::vector<float>> &filter,
                          const int filterSize);
    Vec1D filterVertical(const Vec1D &x, Vec1D& filter, const int filterSize);
    Vec1D transientness(const Vec1D &xHorizontal, const Vec1D &xVertical);

    void deinterleaveRealFFT(Vec1D &dest, const Vec1D &src, int numRealSamples);
    void interleaveRealFFT(Vec1D &dest, const Vec1D &src, int numRealSamples);

    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;

    Vec1D bufferInput;
    Vec1D bufferS;
    Vec1D bufferT;
    Vec1D bufferN;
    Vec1D bufferTN;
    Vec1D bufferTNSmall;

    Vec1D S1;
    Vec1D T1;
    Vec1D N1;

    Vec1D S2;
    Vec1D T2;
    Vec1D N2;

    int newSamplesCount{0};
    int writePtr{0};
    int readPtr{0};
    
    int newSamplesCount2{0};
    int writePtr2{0};

    // Round 1
    juce::dsp::FFT forwardFFT;   // forward S+T+N
    juce::dsp::FFT inverseFFTS;  // inverse S
    juce::dsp::FFT inverseFFTTN; // inverse S

    // Round 2
    juce::dsp::FFT forwardFFTTN; // forward T+N
    juce::dsp::FFT inverseFFTT;  // inverse T
    juce::dsp::FFT inverseFFTN;  // inverse N

    Vec1D fft_1;
    Vec1D fft_1_tn;
    Vec1D real_fft_1;

    Vec1D fft_2;
    Vec1D fft_2_ns;
    Vec1D real_fft_2;

    Vec1D windowS;
    Vec1D windowTN;

    int windowSizeS{8192};
    int windowSizeTN{512};

    const int overlap{8};
    
    int hopSizeS{windowSizeS / overlap};
    int hopSizeTN{windowSizeTN / overlap};

    const float filterLengthTime{0.2f};  // 200ms
    const float filterLengthFreq{500.f}; // 500Hz

    float medFilterHorizontalSizeS{1.f};
    float medFilterVerticalSizeS{1.f};
        
    float medFilterHorizontalSizeTN{1.f};
    float medFilterVerticalSizeTN{1.f};

    Vec1D medianVerticalS;
    Vec1D medianVerticalTN;
    
    std::deque<std::vector<float>> horizontalS;
    std::deque<std::vector<float>> horizontalTN;

    const float threshold_s_1{0.8f};
    const float threshold_s_2{0.7f};
    const float threshold_tn_1{0.85f};
    const float threshold_tn_2{0.75f};
    
    const float windowCorrection{1.f / 3.0f};
};
} // namespace dsp
