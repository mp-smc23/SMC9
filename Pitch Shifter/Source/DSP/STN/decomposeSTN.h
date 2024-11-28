#pragma once
#include "../Helpers/dsp.h"
#include "../MedianFilter/Horizontal/HorizontalMedianFilter.h"
#include "../MedianFilter/Vertical/VerticalMedianFilter.h"
#include <JuceHeader.h>

using Vec2D = std::vector<std::vector<float>>;
using Vec1D = std::vector<float>;

namespace dsp {

struct STN {
    Vec1D S;
    Vec1D T;
    Vec1D N;

    void resize(const size_t newSize) {
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
    void setThresholdSines(const float thresholdLow);
    void setThresholdTransients(const float thresholdLow);

    void process(const juce::AudioBuffer<float> &buffer, juce::AudioBuffer<float> &S, juce::AudioBuffer<float> &T,
                 juce::AudioBuffer<float> &N);
    void prepare();

    int getLatency() const { return fftSizeS + fftSizeTN; }

  private:
    void decompose_1(const int ptr);
    void decompose_2(const int ptr);
    void fuzzySTN(STN &stn, Vec1D &rt, const float G1, const float G2, medianfilter::HorizontalMedianFilter &filterH,
                  medianfilter::VerticalMedianFilter &filterV);

    void transientness(Vec1D &dest, const Vec1D &xHorizontal, const Vec1D &xVertical);

    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;

    Vec1D bufferInput; // buffer for storing incoming samples
    Vec1D bufferS;     // buffer for storing sines from STN 1 step
    Vec1D bufferTN;    // buffer for storing transients and noise from STN 1 step

    Vec1D inputTN; // input buffer for STN 2 step
    Vec1D bufferT; // buffer for storing transients from STN 2 step
    Vec1D bufferN; // buffer for storing noise from STN 2 step

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> sinesDelayLine; // delay for sines, T and N are ready fftSizeTN sample later
    
    STN stn1;
    STN stn2;

    int inputWritePtr{0};          // write pointer for incoming samples
    int bufferSTN1ReadWritePtr{0}; // read write pointer for buffer S and TN
    int inputTNWritePtr{0};        // write pointer for inputTN
    int bufferSTN2ReadWritePtr{0}; // read write pointer for buffer T and N

    int newSamplesCount{0};  // counter for new samples in frame processing STN1
    int newSamplesCount2{0}; // counter for new samples in frame processing STN2

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

    const float filterLengthTime{0.05f}; // 50ms
    const float filterLengthFreq{500.f}; // 500Hz

    medianfilter::HorizontalMedianFilter medianFilterHorS;
    medianfilter::HorizontalMedianFilter medianFilterHorTN;

    medianfilter::VerticalMedianFilter medianFilterVerS;
    medianfilter::VerticalMedianFilter medianFilterVerTN;

    float threshold_s_1{0.8f};
    float threshold_s_2{0.7f};
    float threshold_tn_1{0.85f};
    float threshold_tn_2{0.75f};

    const float windowCorrection{1.f / 3.0f};
};
} // namespace dsp
