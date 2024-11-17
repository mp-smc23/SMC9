#pragma once
#include "../MedianFilterBase.h"

namespace dsp::medianfilter {
class HorizontalMedianFilter final : public MedianFilterBase {
  public:
    void prepare() override {
        result.resize(numSamples, 0.f);
        kernel.resize(filterSize, 0.f);
        filter.resize(filterSize, Vec1D(numSamples, 0.f));
        for(auto& row : filter) row.resize(numSamples, 0.f);
        median.resize(filterSize, 0.f);
    }

    const Vec1D &process(const Vec1D &x) override {
        filter.pop_front();
        filter.push_back(x);

        for (int h = 0; h < numSamples; ++h) {
            for (int i = 0; i < filterSize; ++i) {
                median[i] = filter[i][h];
            }
            // Extract the kernel directly from med_vec using indexing
            juce::FloatVectorOperations::copy(kernel.data(), median.data(), filterSize);

            // Find the median element using nth_element
            std::nth_element(kernel.begin(), kernel.begin() + (pad - 1), kernel.end());
            result[h] = kernel[pad - 1];
        }

        return result;
    };

  private:
    Vec1D median;
    std::deque<Vec1D> filter;
};

} // namespace dsp::medianfilter
