cmake_minimum_required(VERSION 3.16)
project(DynamicAPL25H2 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
enable_testing()
add_executable(apl_estimator_tests tests/estimator_tests.cpp)
target_include_directories(apl_estimator_tests PRIVATE include)
add_test(NAME AplEstimator COMMAND apl_estimator_tests)
add_executable(cube_tests tests/cube_tests.cpp)
target_include_directories(cube_tests PRIVATE include)
add_test(NAME CubeParser COMMAND cube_tests
         "${CMAKE_CURRENT_SOURCE_DIR}/../profiles/HDR_10APL_CALMAN_BASIS_65.cube"
         "${CMAKE_CURRENT_SOURCE_DIR}/../profiles/HDR_2APL_UNKORRIGIERT_GEMESSEN_65.cube")
add_executable(dual_lut_tests tests/dual_lut_tests.cpp)
target_include_directories(dual_lut_tests PRIVATE include)
add_test(NAME TetrahedralAndPQ COMMAND dual_lut_tests
         "${CMAKE_CURRENT_SOURCE_DIR}/../profiles/HDR_10APL_CALMAN_BASIS_65.cube"
         "${CMAKE_CURRENT_SOURCE_DIR}/../profiles/HDR_2APL_UNKORRIGIERT_GEMESSEN_65.cube")
