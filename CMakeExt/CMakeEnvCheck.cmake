# Check if the CMake version is high enought to support all features
# otherwise print warnings

# target exports for using findPackage(dash)
if("${CMAKE_VERSION}" VERSION_LESS 3.0.0)
	message(WARNING "To use target exports use at least CMake 3.0.0")
endif()
