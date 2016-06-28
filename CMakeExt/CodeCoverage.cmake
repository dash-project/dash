message(STATUS "Downloading CodeCoverage CMake integration script from github repository of user bilke")
set(CODE_COVERAGE_PREFIX "${CMAKE_BINARY_DIR}/code_coverage")
file(
  DOWNLOAD
  https://raw.githubusercontent.com/bilke/cmake-modules/master/CodeCoverage.cmake
  ${CODE_COVERAGE_PREFIX}/CodeCoverage.cmake
  )

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CODE_COVERAGE_PREFIX})
include(CodeCoverage)
