//#pragma once
//#include "r8brain/CDSPResampler.h"
//#include <JuceHeader.h>
//
//using Vec1D = std::vector<float>;
//
//namespace dsp {
//class Resample {
//  public:
//    Resample() : resampler{std::make_shared<r8b::CDSPResampler>(srcSampleRate, dstSampleRate, maxInLen)} {};
//
//    void process(const Vec1D &input, Vec1D &output);
//    void prepare();
//
//    void setSrcSampleRate(double rate);
//    void setDstSampleRate(double rate);
//    void setMaxInLen(int len);
//
//  private:
//    float ratio{88000.};
//
//    
//};
//
//} // namespace dsp
