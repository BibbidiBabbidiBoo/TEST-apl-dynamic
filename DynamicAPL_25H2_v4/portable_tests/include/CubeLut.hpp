#pragma once
// Safe, platform-independent parser for standard RGB .cube 3D LUTs.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <istream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace dynapl {
struct CubeLut {
    unsigned size=0;
    std::vector<std::array<float,4>> rgba;  // red-fastest, then green, then blue
};
inline CubeLut ReadCube(std::istream& input) {
    CubeLut out;
    bool sawMin=false,sawMax=false;
    std::string line;
    while(std::getline(input,line)) {
        auto comment=line.find('#');
        if(comment!=std::string::npos)line.erase(comment);
        std::istringstream ss(line);
        std::string field;
        if(!(ss>>field))continue;
        if(field=="TITLE")continue;
        if(field=="LUT_1D_SIZE")throw std::runtime_error("Only 3D .cube LUTs are supported");
        if(field=="LUT_3D_SIZE") {
            if(out.size || !out.rgba.empty() || !(ss>>out.size) || out.size<2 || out.size>65)
                throw std::runtime_error("Unsupported or duplicate LUT size");
            out.rgba.reserve(static_cast<size_t>(out.size)*out.size*out.size);
            continue;
        }
        if(field=="DOMAIN_MIN" || field=="DOMAIN_MAX") {
            float v[3];
            if(!(ss>>v[0]>>v[1]>>v[2]))throw std::runtime_error("Bad domain");
            const float expected=field=="DOMAIN_MIN"?0.0f:1.0f;
            for(float x:v)if(!std::isfinite(x) || std::abs(x-expected)>0.0001f)
                throw std::runtime_error("Only 0..1 domain supported");
            if(field=="DOMAIN_MIN")sawMin=true;else sawMax=true;
            continue;
        }
        if(out.size==0) throw std::runtime_error("Expected LUT_3D_SIZE before rows");
        std::istringstream row(line);
        std::array<float,4> sample{};
        if(!(row>>sample[0]>>sample[1]>>sample[2]))
            throw std::runtime_error("Bad LUT RGB row");
        for(unsigned c=0;c<3;c++)if(!std::isfinite(sample[c]) || sample[c]<0 || sample[c]>1)
            throw std::runtime_error("LUT values must be in [0,1]");
        sample[3]=1.0f;
        out.rgba.push_back(sample);
        if(out.rgba.size()>static_cast<size_t>(out.size)*out.size*out.size)
            throw std::runtime_error("Too many LUT rows");
    }
    if(out.size==0 || out.rgba.size()!=static_cast<size_t>(out.size)*out.size*out.size)
        throw std::runtime_error("Missing LUT rows");
    // Common domain defaults are exactly 0 and 1 if DOMAIN_* omitted.
    (void)sawMin;(void)sawMax;
    return out;
}
} // namespace dynapl
