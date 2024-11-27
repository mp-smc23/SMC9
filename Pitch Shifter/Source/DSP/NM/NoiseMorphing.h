#pragma once
#include "../Helpers/NoiseGenerator.h"
#include "../Helpers/dsp.h"
#include <JuceHeader.h>

using Vec1D = std::vector<float>;
using Vec2D = std::vector<std::vector<float>>;

namespace dsp {
class NoiseMorphing {
  public:
    NoiseMorphing(std::shared_ptr<juce::dsp::ProcessSpec> procSpec);
    ~NoiseMorphing() = default;

    /// Set pitch shift in semitones. The pitch shift ratio is calculated as 2^(semitones/12). Supported range is -12
    /// to 24.
    /// - Parameter semitones: Semitones to shift.
    void setPitchShiftSemitones(const int semitones);

    /// Set pitch shift ratio. Supported range is 0.5  to 4.
    /// - Parameter newPitchShiftRatio: Pitch shift ratio.
    void setPitchShiftRatio(const float newPitchShiftRatio);

    /// Set FFT size. Resized all the interal storage buffers. Not audio thread safe.
    /// - Parameter newFFTSize: New FFT size.
    void setFFTSize(const int newFFTSize);

    /// Prepares and resized all internal buffers for processing. Must be called at least once before processing start.
    void prepare();

    /// Process incoming audio buffer.
    /// - Parameter buffer: Audio buffer to process.
    void process(juce::AudioBuffer<float> &buffer);

    int getLatency() const { return fftSize + interpolator.getBaseLatency(); }

  private:
    /// Process a single frame of audio. As a result output buffer gets filled with new samples.
    void processFrame();

    /// Runs frame interpolation in order to stretch the spectrum. Resulting frames are saved to dest. The number of
    /// resulting frames depends on the current pitch shift ratio.
    /// - Parameters:
    ///   - dest: Destination vector. Will be filled with the interpolated frames.
    ///   - frame1: Previous frame.
    ///   - frame2: Current frame.
    void stretchSpectrum(Vec2D &dest, const Vec1D &frame1, const Vec1D &frame2);

    /// Generates chunk of normalized white noise and appends it to white noise buffer.
    /// - Parameter size: Size of generated noise chunk.
    void generateNoise(int size);

    /// Perform noise morphing. Destination vector is modulated by the noise spectrum via element-wise multiplication.
    /// - Parameters:
    ///   - dest: Vector to be modulated.
    void noiseMorphing(Vec1D &dest);

    /// Interpolates two frames of a spectrum at a given fractional point.
    /// - Parameters:
    ///   - dest: Resulting frame.
    ///   - frame1: Previous frame.
    ///   - frame2: Current frame.
    ///   - frac: Point of interpolation.
    static void interpolateFrames(Vec1D &dest, const Vec1D &frame1, const Vec1D &frame2, const float frac);

    // ===== FFT Params =====
    int fftSize{2048};
    const int overlap{2};

    int hopSize{fftSize / overlap};
    int hopSizeStretch{512};

    const float windowCorrection{4.f / 3.0f}; // overlap 50%
    float windowCorrectionStretch{4.f / 3.0f};
    float windowEnergy{1.f};

    int spectrumInterpolationFrames{3};

    // ==== Pitch shift ====
    float pitchShiftRatio{2.f}; // linear
    const int maxPitchShiftRatio{4};

    // ==== Noise ====
    helpers::NoiseGenerator noiseGenerator;

    juce::dsp::FFT forwardFFT;
    juce::dsp::FFT forwardFFTNoise;
    juce::dsp::FFT inverseFFT;

    Vec1D window;            // hann window
    Vec1D input;             // input buffer for storing incoming audio samples
    Vec1D whiteNoise;        // generated white noise
    Vec1D stretched;         // stretched noise - circular
    Vec1D stretchedUnwinded; // stretched noise - when filled next sample is always at idx 0
    Vec1D output;            // resampled output buffer

    Vec1D fft;        // buffer for fft processing of input
    Vec1D fftAbs;     // absolute values of the fft
    Vec1D fftAbsPrev; // previous absolute values of the fft
    Vec1D fftNoise;   // buffer for fft processing of noise

    Vec2D interpolatedFrames; // interpolated spectral frames

    int newSamplesCount{0};   // counter for new samples in frame processing
    int writeReadPtrInput{0}; // input buffer write/read pointer
    int writeReadPtrNoise{0}; // noise buffer write/read pointer
    int writePtrStretched{0}; // stretched noise write pointer
    int readPtrStretched{0};  // stretched noise read pointer

    juce::WindowedSincInterpolator interpolator;

    std::shared_ptr<juce::dsp::ProcessSpec> processSpec;
};
} // namespace dsp
