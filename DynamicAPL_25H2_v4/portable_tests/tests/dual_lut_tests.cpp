#include "DualLutReference.hpp"
// Keep test assertions active even when Release defines NDEBUG.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <fstream>
using namespace dynapl;
static CubeLut Identity(unsigned n) {
    CubeLut out;out.size=n;
    for(unsigned b=0;b<n;b++)for(unsigned g=0;g<n;g++)for(unsigned r=0;r<n;r++)
        out.rgba.push_back({float(r)/(n-1),float(g)/(n-1),float(b)/(n-1),1.f});
    return out;
}
static float dist(const RGB& a,const RGB& b) {
    return std::max({std::abs(a[0]-b[0]),std::abs(a[1]-b[1]),std::abs(a[2]-b[2])});
}
int main(int argc,char** argv) {
    auto identity=Identity(65);
    std::mt19937 rng(1234);
    std::uniform_real_distribution<float> uniform(0,1);
    for(int i=0;i<20000;i++) {
        RGB rgb{uniform(rng),uniform(rng),uniform(rng)};
        assert(dist(rgb,TetraSample(identity,rgb))<1e-5f);
        auto blended=DualLutSample(identity,identity,rgb,uniform(rng));
        if(dist(rgb,blended)>=2e-4f){std::cerr << "FAIL PQ roundtrip rgb="<<rgb[0]<<","<<rgb[1]<<","<<rgb[2]<<" got "<<blended[0]<<","<<blended[1]<<","<<blended[2]<<" dist="<<dist(rgb,blended)<<"\n"; return 1;}
    }
    for(int x=0;x<=100;x+=5) {
        auto rgb=RGB{float(x)/100,float(x)/100,float(x)/100};
        assert(dist(rgb,TetraSample(identity,rgb))<1e-5f);
        float pq=float(x)/100.f;
        assert(std::abs(PQEncode10000(PQDecode10000(pq))-pq)<0.0002f);
    }
    assert(argc==3); // actual measured .cube files
    auto read=[](const char* f) {
        std::ifstream stream(f);
        if(!stream)throw std::runtime_error("Missing cube file");
        return ReadCube(stream);
    };
    const auto ten=read(argv[1]),two=read(argv[2]);
    assert(ten.size==65 && two.size==65);
    for(int x=0;x<=100;x+=5) {
        float v=x/100.f; RGB gray{v,v,v};
        auto a=TetraSample(ten,gray), b=TetraSample(two,gray);
        assert(dist(a,DualLutSample(ten,two,gray,0))<0.0002f);
        assert(dist(b,DualLutSample(ten,two,gray,1))<0.0002f);
        auto half=DualLutSample(ten,two,gray,.5f);
        for(int k=0;k<3;k++) {
            auto mid=PQDecode10000(half[k]);
            assert(mid>=std::min(PQDecode10000(a[k]),PQDecode10000(b[k]))-1e-6f);
            assert(mid<=std::max(PQDecode10000(a[k]),PQDecode10000(b[k]))+1e-6f);
        }
    }
    std::cout<<"PASS: identity, R-fastest indexing, tetrahedral six cases, PQ round trip, dual-blend endpoints\n";
}
