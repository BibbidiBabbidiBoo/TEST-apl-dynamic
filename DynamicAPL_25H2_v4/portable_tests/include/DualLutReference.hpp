#pragma once
// C++17 independent reference for validating the HLSL tetrahedral sampling.
// It is NOT a dwm_lut DLL and does not modify the desktop.
#include "CubeLut.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace dynapl {
using RGB=std::array<float,3>;
inline RGB TetraSample(const CubeLut& lut, const RGB& rgb) {
    if(lut.size<2 || lut.rgba.size()!=static_cast<size_t>(lut.size)*lut.size*lut.size)
        throw std::runtime_error("LUT size/data mismatch");
    const unsigned maxIndex=lut.size-2;
    unsigned x[3];float f[3];
    for(int k=0;k<3;k++) {
        const float pos=std::clamp(rgb[k],0.f,1.f)*static_cast<float>(lut.size-1);
        x[k]=std::min(static_cast<unsigned>(pos),maxIndex);
        f[k]=pos-static_cast<float>(x[k]);
    }
    auto at=[&](unsigned a,unsigned b,unsigned c)->RGB {
        const auto& p=lut.rgba[(static_cast<size_t>(x[2]+c)*lut.size + (x[1]+b))*lut.size + x[0]+a];
        return {p[0],p[1],p[2]};
    };
    const RGB a=at(0,0,0), h=at(1,1,1);
    RGB b,c;float w[3];
    if (f[0]>=f[1]) {
        if(f[1]>=f[2]) {b=at(1,0,0);c=at(1,1,0);w[0]=f[0];w[1]=f[1];w[2]=f[2];}
        else if(f[0]>=f[2]) {b=at(1,0,0);c=at(1,0,1);w[0]=f[0];w[1]=f[2];w[2]=f[1];}
        else {b=at(0,0,1);c=at(1,0,1);w[0]=f[2];w[1]=f[0];w[2]=f[1];}
    } else {
        if(f[0]>=f[2]) {b=at(0,1,0);c=at(1,1,0);w[0]=f[1];w[1]=f[0];w[2]=f[2];}
        else if(f[1]>=f[2]) {b=at(0,1,0);c=at(0,1,1);w[0]=f[1];w[1]=f[2];w[2]=f[0];}
        else {b=at(0,0,1);c=at(0,1,1);w[0]=f[2];w[1]=f[1];w[2]=f[0];}
    }
    RGB result;
    for(int k=0;k<3;k++) result[k]=a[k]+w[0]*(b[k]-a[k])+w[1]*(c[k]-b[k])+w[2]*(h[k]-c[k]);
    return result;
}
inline float PQDecode10000(float x) {
    constexpr float m1=2610.f/16384.f,m2=2523.f/32.f,c1=3424.f/4096.f,c2=2413.f/128.f,c3=2392.f/128.f;
    float p=std::pow(std::clamp(x,0.f,1.f),1.f/m2);
    return std::pow(std::max(p-c1,0.f)/std::max(c2-c3*p,1e-7f),1.f/m1);
}
inline float PQEncode10000(float x) {
    constexpr float m1=2610.f/16384.f,m2=2523.f/32.f,c1=3424.f/4096.f,c2=2413.f/128.f,c3=2392.f/128.f;
    float p=std::pow(std::clamp(x,0.f,1.f),m1);
    return std::pow((c1+c2*p)/(1.f+c3*p),m2);
}
inline RGB DualLutSample(const CubeLut& lut10,const CubeLut& lut2,const RGB& pq,float blend2) {
    auto a=TetraSample(lut10,pq),b=TetraSample(lut2,pq);
    RGB out{};
    for(int k=0;k<3;k++)out[k]=PQEncode10000(
        (1-std::clamp(blend2,0.f,1.f))*PQDecode10000(a[k])+
        std::clamp(blend2,0.f,1.f)*PQDecode10000(b[k]));
    return out;
}
} // namespace dynapl
