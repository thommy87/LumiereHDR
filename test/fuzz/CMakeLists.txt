# libFuzzer targets for the image readers (clang only).
# Build with -DLHDR_BUILD_FUZZERS=ON -DCMAKE_CXX_COMPILER=clang++

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "LHDR_BUILD_FUZZERS needs clang (libFuzzer)")
endif()

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(fuzz_rgbe fuzz_rgbe.cpp)
target_compile_options(fuzz_rgbe PRIVATE -fsanitize=fuzzer,address,undefined)
target_link_libraries(fuzz_rgbe pfs ${LIBS} -fsanitize=fuzzer,address,undefined)
