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

    int getLatency() const { return fftSize + (interpolator.getBaseLatency()) / pitchShiftRatio; }
    
  private:
    
    void processFrame();
    void framesInterpolation();
    void generateNoise(int size);
    void noiseMorphing(Vec1D& dest, const int ptr);
    
    int fftSize{2048};
    int overlap{2};
    int hopSize{fftSize / overlap};
    float windowEnergy{1.f};
    
    float pitchShiftRatio{2.f}; // linear
    const int maxPitchShiftRatio{2};
    int hopSizeStretch{512};
    float windowCorrectionStretch{4.f / 3.0f};
    

    // Noise
    // Define random generator with Gaussian distribution
    const float mean = 0.0f;
    const float stddev = 1.f;
    std::mt19937 generator{std::random_device{}()};
    std::normal_distribution<float> dist{mean, stddev};
    
    juce::dsp::FFT forwardFFT;
    juce::dsp::FFT forwardFFTNoise;
    juce::dsp::FFT inverseFFT;
    
    Vec1D window;
    Vec1D input;
    Vec1D whiteNoise;
    Vec1D output;
    Vec1D fft;
    Vec1D fftAbs;
    Vec1D fftAbsPrev;
    Vec1D fftNoise;
    
    std::vector<Vec1D> interpolatedFrames;
    Vec1D stretched;
    Vec1D stretchedUnwinded;
    
    int newSamplesCount{0}; // ok
    int writeReadPtrInput{0}; //ok
    int writeReadPtrNoise{0}; // ok
    int writePtrStretched{0}; //ok
    int readPtrStretched{0}; //ok
    
    juce::WindowedSincInterpolator interpolator;
    
    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;
    
    const float windowCorrection{4.f / 3.0f}; // overlap 50%
    
    
    
};
} // namespace dsp

