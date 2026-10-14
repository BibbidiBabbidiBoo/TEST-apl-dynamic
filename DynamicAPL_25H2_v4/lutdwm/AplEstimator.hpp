#pragma once
// Dual-LUT APL decision core (standard C++17, no Windows dependencies).
// (std::min)/(std::max) intentionally bypass Windows min/max preprocessor macros.
// For 2% vs 10% white-window calibration, these are SCENE statistics,
// not knowledge of the physical Mini-LED backlight zones.
#include <array>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace dynapl {
constexpr unsigned kBins = 32;
constexpr unsigned kWidth = 256;
constexpr unsigned kHeight = 144;
constexpr float kMinLog2Nits = -4.0f;
constexpr float kBinStepLog2 = 0.5f;
using Histogram = std::array<uint32_t, kBins>;

inline unsigned HistogramBin(float nits) {
    if (!std::isfinite(nits) || nits <= 0.0f) return 0;
    int idx = static_cast<int>(std::floor((std::log2(nits) - kMinLog2Nits)/kBinStepLog2));
    return static_cast<unsigned>(std::clamp(idx,0,static_cast<int>(kBins-1)));
}
inline float LowerBinNits(unsigned bin) {
    return std::exp2(kMinLog2Nits + bin*kBinStepLog2);
}
struct FrameStats {
    bool valid = false;
    float brightFraction = 0.0f;
    float p995NitsEstimate = 0.0f;
    uint32_t samples = 0;
};
inline FrameStats AnalyzeHistogram(const Histogram& hist) {
    FrameStats out;
    uint64_t total=0;
    for (auto n:hist) total+=n;
    if (total<256) return out;
    out.samples=static_cast<uint32_t>(total);
    const uint64_t target=static_cast<uint64_t>(std::floor((total-1)*0.995));
    uint64_t cumulative=0;
    unsigned percentileBin=kBins-1;
    for(unsigned b=0;b<kBins;b++) {
        cumulative+=hist[b];
        if(cumulative>target) {percentileBin=b;break;}
    }
    // Upper bound reduces the risk of underestimating the threshold.
    out.p995NitsEstimate = LowerBinNits(percentileBin) * std::sqrt(2.0f);
    // Do not force the 2% profile for dark frames or SDR-only scenes.
    if(out.p995NitsEstimate < 5.0f) return out;
    const float cutoffNits=(std::max)(4.0f, 0.20f*out.p995NitsEstimate);
    double bright=0;
    for(unsigned b=0;b<kBins;b++) {
        float lo=LowerBinNits(b), hi=LowerBinNits(b+1);
        if(lo>=cutoffNits) bright+=hist[b];
        else if(hi>cutoffNits) { // fractional bin approximation
            const float fraction=(std::log2(hi)-std::log2(cutoffNits)) / kBinStepLog2;
            bright+=hist[b]*std::clamp(fraction,0.0f,1.0f);
        }
    }
    out.brightFraction=static_cast<float>(bright/total);
    // Less than 0.5% eligible highlights is too ambiguous (pixels, UI etc).
    out.valid=out.brightFraction>=0.005f;
    return out;
}
inline Histogram MakeCpuReferenceHistogram(const float* rgba, unsigned width, unsigned height,
                                           unsigned stridePixels=4) {
    Histogram h{};
    if (!rgba||width==0||height==0||stridePixels==0) return h;
    for(unsigned y=0;y<kHeight;y++) for(unsigned x=0;x<kWidth;x++) {
        const unsigned sy=(std::min)(height-1,static_cast<unsigned>((y+0.5f)*height/kHeight));
        const unsigned sx=(std::min)(width-1,static_cast<unsigned>((x+0.5f)*width/kWidth));
        const float* p=rgba+(static_cast<size_t>(sy)*stridePixels*width+static_cast<size_t>(sx)*stridePixels);
        // scRGB reference 1.0 = 80 cd/m2, linear Rec.709 primaries.
        const float yNits=(std::max)(0.0f,80.0f*(.2126f*p[0]+.7152f*p[1]+.0722f*p[2]));
        h[HistogramBin(yNits)]++;
    }
    return h;
}
inline float SmoothStep(float low,float high,float x) {
    const float t=std::clamp((x-low)/(high-low),0.0f,1.0f);
    return t*t*(3-2*t);
}
struct Controller {
    float mix2 = 0.0f;                 // zero: 10% LUT; one: 2% LUT
    float smoothingSeconds=1.5f;
    float maxChangePerSecond=.35f;
    float lowerCoverage=.018f;
    float upperCoverage=.105f;
    float Update(const FrameStats& s,float dt) {
        dt=std::clamp(dt,0.001f,2.0f);
        // No trustworthy highlight means prefer conservative 10% reference.
        const float desired=s.valid ? 1.0f-SmoothStep(lowerCoverage,upperCoverage,s.brightFraction) : 0.0f;
        const float alpha=1.0f-std::exp(-dt/(std::max)(smoothingSeconds,.01f));
        const float filtered=mix2+alpha*(desired-mix2);
        const float step=maxChangePerSecond*dt;
        mix2=std::clamp(filtered,(std::max)(0.0f,mix2-step),(std::min)(1.0f,mix2+step));
        return mix2;
    }
    void Reset(){mix2=0.0f;}
};
} // namespace dynapl
