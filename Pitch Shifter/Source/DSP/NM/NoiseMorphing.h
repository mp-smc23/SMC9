#pragma once
#include <JuceHeader.h>
#include "../Helpers/Interleave.h"
#include <random>

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
    void noiseMorphing(Vec1D& dest, const int ptr);
    
    int fftSize{2048};
    int overlap{2};
    int hopSize{fftSize / overlap};
    float windowEnergy{1.f};
    
    float pitchShiftRatio{2.f}; // linear
    const int maxPitchShiftRatio{2};

    // Noise
    // Define random generator with Gaussian distribution
    const float mean = 0.0;
    const float stddev = 0.1;
    std::mt19937 generator{std::random_device{}()};
    std::normal_distribution<float> dist{mean, stddev};
    
    juce::dsp::FFT forwardFFT;
    juce::dsp::FFT forwardFFTNoise;
    juce::dsp::FFT inverseFFT;
    Vec1D window;

    Vec1D input;
    Vec1D whiteNoise;
    Vec1D bufferOutput;
    Vec1D fft;
    Vec1D fftAbs;
    Vec1D fftAbsPrev;
    Vec1D fftNoise;
    
    Vec1D stretched;
    
    int newSamplesCount{0};
    int writePtr{0};
    int readPtr{0};

    juce::GenericInterpolator<juce::WindowedSincInterpolator, 2048> interpolator;
    
    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;
    
    const float windowCorrection{4.f / 3.0f}; // overlap 50%
};
} // namespace dsp

