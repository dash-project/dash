# Check if the CMake version is high enought to support all features
# otherwise print warnings and disable these features

# target exports for using findPackage(dash)
if("${CMAKE_VERSION}" VERSION_LESS 3.0.0)
  message(WARNING "To use target exports use at least CMake 3.0.0")
endif()

# disable build time reduction if cmake version is to low
if(ENABLE_COMPTIME_RED)
  if(("${CMAKE_VERSION}" VERSION_LESS 2.8.12))
    message(WARNING "Build time reduction no supported by this cmake version")
    set(ENABLE_COMPTIME_RED OFF)
  endif()
endif()
