#pragma once
#include <JuceHeader.h>
#include "../Helpers/Interleave.h"

using Vec1D = std::vector<float>;

namespace dsp {
class NoiseMorphing {
  public:
    NoiseMorphing(std::shared_ptr<juce::dsp::ProcessSpec> procSpec);
    ~NoiseMorphing() = default;

    void setPitchShiftSemitones(const int semitones);
    void setFFTSize(const int newFFTSize);
    
    void prepare();
    void process(juce::AudioBuffer<float>& buffer);

  private:
    
    void processFrame();
    void framesInterpolation();
    void noiseMorphing(Vec1D& dest);
    
    int fftSize{2048};
    int overlap{2};
    int hopSize{fftSize / overlap};
    
    float pitchShiftRatio{2.f}; // linear

    juce::dsp::FFT forwardFFT;
    juce::dsp::FFT forwardFFTNoise;
    juce::dsp::FFT inverseFFT;
    Vec1D window;

    Vec1D input;
    Vec1D bufferOutput;
    Vec1D fft;
    Vec1D fftAbs;
    Vec1D fftAbsPrev;
    Vec1D fftNoise;
    
    Vec1D stretched;
    
    int newSamplesCount{0};
    int writePtr{0};
    int readPtr{0};

    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;
    
    const float windowCorrection{4.f / 3.0f}; // overlap 50%
};
} // namespace dsp

