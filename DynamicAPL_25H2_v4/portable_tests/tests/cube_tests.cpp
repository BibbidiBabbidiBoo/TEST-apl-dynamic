#include "CubeLut.hpp"
// Keep test assertions active even when Release defines NDEBUG.
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
int main(int argc, char** argv) {
    const std::string test="TITLE \"test\"\nLUT_3D_SIZE 2\nDOMAIN_MIN 0 0 0\nDOMAIN_MAX 1 1 1\n"
        "0 0 0\n1 0 0\n0 1 0\n1 1 0\n0 0 1\n1 0 1\n0 1 1\n1 1 1\n";
    std::istringstream ss(test);
    auto lut=dynapl::ReadCube(ss);
    assert(lut.size==2 && lut.rgba.size()==8 && lut.rgba[1][0]==1.f && lut.rgba[4][2]==1.f);
    bool caught=false;
    try {std::istringstream bad("LUT_3D_SIZE 2\n0 0 0\n");dynapl::ReadCube(bad);}
    catch(...) {caught=true;}
    assert(caught);
    for(int i=1;i<argc;i++){
       std::ifstream f(argv[i]);assert(f);
       auto real=dynapl::ReadCube(f);
       assert(real.size==65 && real.rgba.size()==65u*65*65);
       std::cout << "OK CUBE: " << argv[i] << " (" << real.rgba.size() << " RGB rows)\n";
    }
    std::cout << "PASS: parser and malformed files\n";
}
