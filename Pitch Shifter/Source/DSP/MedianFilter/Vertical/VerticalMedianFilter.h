#pragma once
#include "../MedianFilterBase.h"

namespace dsp::medianfilter {
class VerticalMedianFilter final : public MedianFilterBase {
  public:
    void prepare() override {
        result.resize(numSamples, 0.f);
        kernel.resize(filterSize, 0.f);
        filter.resize(numSamples + filterSize, 0.f);
    };

    const Vec1D &process(const Vec1D &x) override {
        juce::FloatVectorOperations::copy(filter.data() + pad, x.data(), x.size());
        for (auto h = 0; h < numSamples; h++) {
            juce::FloatVectorOperations::copy(kernel.data(), filter.data() + h, filterSize);

            // Find the median element using nth_element
            std::nth_element(kernel.begin(), kernel.begin() + pad, kernel.end());
            result[h] = kernel[pad];
        }

        return result;
    };

  private:
    Vec1D filter;
};

} // namespace dsp::medianfilter
