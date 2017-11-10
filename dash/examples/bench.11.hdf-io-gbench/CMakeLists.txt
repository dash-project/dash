message(STATUS "Benchmark ${exampletarget} requires Google Benchmark")
include(${CMAKE_SOURCE_DIR}/CMakeExt/GoogleBenchmark.cmake)

set(target_libraries "GBench"             PARENT_SCOPE)
set(target_includes  "${GBENCH_INCLUDES}" PARENT_SCOPE)

