//#include "Resample.h"
//
//
//void dsp::Resample::prepare() {
//    resampler = std::make_shared<r8b::CDSPResampler>(srcSampleRate, dstSampleRate, maxInLen);
//    DBG("Resampler prepared");
//    DBG("Parameters: SRC sample rate" + juce::String(srcSampleRate) + " | DESC sample rate " + juce::String(dstSampleRate) + " | maxInLen " + juce::String(maxInLen));
//    DBG("getInputRequiredForOutput(maxInLen) = " + juce::String(resampler->getInputRequiredForOutput(maxInLen)));
//    DBG("getLatency() = " + juce::String(resampler->getLatency()));
//    DBG("getLatencyFrac() = " + juce::String(resampler->getLatencyFrac()));
//}
//
//void dsp::Resample::process(const Vec1D &input, Vec1D &output) {
//    // One shot version
//    resampler->oneshot(input.data(), input.size(), output.data(), output.size() / 2);
//    
//    // Process version
////    const auto resultLen = resampler->process(input.data(), input.size(), output.data());
////    DBG("Result len = " + juce::String(resultLen));
//}
//
//
//void dsp::Resample::setSrcSampleRate(double rate) {
//    if(rate == srcSampleRate) return;
//    srcSampleRate = rate;
//    prepare();
//}
//
//void dsp::Resample::setDstSampleRate(double rate) {
//    if(rate == dstSampleRate) return;
//    dstSampleRate = rate;
//    prepare();
//}
//
//void dsp::Resample::setMaxInLen(int len) {
//    if(len == maxInLen) return;
//    maxInLen = len;
//    prepare();
//}
