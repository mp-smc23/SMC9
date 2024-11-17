#pragma once
#include <JuceHeader.h>
#include "../Helpers/Interleave.h"
#include "../MedianFilter/Vertical/VerticalMedianFilter.h"
#include "../MedianFilter/Horizontal/HorizontalMedianFilter.h"

using Vec2D = std::vector<std::vector<float>>;
using Vec1D = std::vector<float>;

namespace dsp {

    struct STN{
        Vec1D S;
        Vec1D T;
        Vec1D N;
        
        void resize(const size_t newSize){
            S.resize(newSize, 0.f);
            T.resize(newSize, 0.f);
            N.resize(newSize, 0.f);
        }
    };
    
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
    void fuzzySTN(STN& stn, Vec1D& rt,
                  const float G1, const float G2,
                  medianfilter::HorizontalMedianFilter& filterH,
                  medianfilter::VerticalMedianFilter& filterV);

    void transientness(Vec1D& dest, const Vec1D &xHorizontal, const Vec1D &xVertical);

    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;

    Vec1D bufferInput;
    Vec1D bufferS;
    Vec1D bufferT;
    Vec1D bufferN;
    Vec1D bufferTN;
    Vec1D bufferTNSmall;

    STN stn1;
    STN stn2;

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
    Vec1D real_fft_1_s;
    Vec1D real_fft_1_tn;
    Vec1D imag_fft_1_s;
    Vec1D imag_fft_1_tn;

    Vec1D fft_2;
    Vec1D fft_2_ns;
    Vec1D real_fft_2_t;
    Vec1D imag_fft_2_t;
    Vec1D real_fft_2_ns;
    Vec1D imag_fft_2_ns;
    
    Vec1D rtS;
    Vec1D rtTN;

    Vec1D windowS;
    Vec1D windowTN;

    int fftSizeS{2048};
    int fftSizeTN{512};

    const int overlap{8};
    
    int hopSizeS{fftSizeS / overlap};
    int hopSizeTN{fftSizeTN / overlap};

    const float filterLengthTime{0.05f};  // 50ms
    const float filterLengthFreq{500.f}; // 500Hz
    
    medianfilter::HorizontalMedianFilter medianFilterHorS;
    medianfilter::HorizontalMedianFilter medianFilterHorTN;
    
    medianfilter::VerticalMedianFilter medianFilterVerS;
    medianfilter::VerticalMedianFilter medianFilterVerTN;

    const float threshold_s_1{0.8f};
    const float threshold_s_2{0.7f};
    const float threshold_tn_1{0.85f};
    const float threshold_tn_2{0.75f};
    
    const float windowCorrection{1.f / 3.0f};
};
} // namespace dsp
