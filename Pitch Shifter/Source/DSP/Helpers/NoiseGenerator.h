#pragma once
#include <JuceHeader.h>
#include <random>

using Vec1D = std::vector<float>;

namespace dsp::helpers {
class NoiseGenerator {

  public:
    NoiseGenerator() = default;
    
    void prepare(int maxSize){
        whiteNoise.resize(maxSize);
    }

    const Vec1D &generateNormalizedNoise(int size) {
        auto maxNoise = 1.f;
        for (auto i = 0; i < size; i++) {
            whiteNoise[i] = dist(generator);
            if (const auto absNoise = std::abs(whiteNoise[i]); absNoise > maxNoise) {
                maxNoise = absNoise;
            }
        }
        juce::FloatVectorOperations::multiply(whiteNoise.data(), 1.f / maxNoise, size);
        return whiteNoise;
    };

  private:
    Vec1D whiteNoise;
    // Define random generator with Gaussian distribution
    const float mean = 0.0f;
    const float stddev = 1.f;
    std::mt19937 generator{std::random_device{}()};
    std::normal_distribution<float> dist{mean, stddev};
};
} // namespace dsp
