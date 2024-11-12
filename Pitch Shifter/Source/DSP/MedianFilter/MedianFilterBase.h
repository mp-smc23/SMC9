#pragma once
using Vec1D = std::vector<float>;

namespace dsp::medianfilter {
class MedianFilterBase {
  public:
    MedianFilterBase() = default;

    void setFilterSize(const int newFilterSize) {
        if(filterSize == newFilterSize) return;
        filterSize = newFilterSize;
        pad = static_cast<int>(std::floorf(filterSize / 2.f));
        
        DBG("New Filter Size = " + juce::String(newFilterSize));
        DBG("Pad = " + juce::String(pad));
        prepare();
    };

    void setSamplesSize(const int newSamplesSize) {
        if(numSamples == newSamplesSize) return;
        numSamples = newSamplesSize;
        
        DBG("New Samples (Window Size) = " + juce::String(newSamplesSize));
        prepare();
    };

    virtual void prepare() = 0;
    virtual const Vec1D &process(const Vec1D& x) = 0;

  protected:
    int filterSize{2};
    int numSamples{1};
    int pad{1};

    Vec1D result;
    Vec1D kernel;
};
} // namespace dsp::medianfilter
