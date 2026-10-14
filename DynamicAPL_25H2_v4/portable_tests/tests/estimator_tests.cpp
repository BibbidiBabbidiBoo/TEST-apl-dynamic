#include "AplEstimator.hpp"
// Keep test assertions active even when Release defines NDEBUG.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace dynapl;

static FrameStats Window(float coverage, float foregroundNits=700.0f) {
    const unsigned w=256,h=144;
    std::vector<float> pix(w*h*4,0.0f);
    const unsigned count=static_cast<unsigned>(w*h*coverage);
    for(unsigned i=0;i<w*h;i++) {
        float v=(i<count?foregroundNits:0.002f)/80.0f;
        for(int c=0;c<3;c++) pix[4*i+c]=v;
        pix[4*i+3]=1.0f;
    }
    return AnalyzeHistogram(MakeCpuReferenceHistogram(pix.data(),w,h));
}
int main() {
    const auto a2=Window(.02f), a10=Window(.10f), a0=Window(0), a05=Window(.005f);
    std::cout << "2%: " << a2.brightFraction << " 10%: " << a10.brightFraction
              << " 0% valid: " << a0.valid << '\n';
    assert(a2.valid&&a10.valid);
    assert(std::abs(a2.brightFraction-.02f)<.002f);
    assert(std::abs(a10.brightFraction-.10f)<.002f);
    assert(!a0.valid);
    Controller ctrl;
    for(int i=0;i<40;i++) ctrl.Update(a2,.25f);
    assert(ctrl.mix2>.96f);
    float prev=ctrl.mix2;
    ctrl.Update(a10,.25f);
    assert(prev-ctrl.mix2 <= .35f*.25f + 1e-6f); // rate limit
    for(int i=0;i<40;i++) ctrl.Update(a10,.25f);
    assert(ctrl.mix2 < .04f);
    for(int i=0;i<40;i++) ctrl.Update(a2,.25f);
    for(int i=0;i<40;i++) ctrl.Update(a0,.25f);
    assert(ctrl.mix2 < .04f); // dark scene MUST NOT lock 2% profile
    ctrl.Reset(); assert(ctrl.mix2==0.0f);
    std::cout << "PASS: scRGB histogram, 2/10 percent classification, smoothing, fallback\n";
}
