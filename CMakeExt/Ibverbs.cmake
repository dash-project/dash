# This module will set the following variables:
#   IBVERBS_FOUND                TRUE if we have found ibverbs
#   IBVERBS_C_LIBRARIES          Libraries for GASPI
#   IBVERBS_LINK_FLAGS           Linking flags for GASPI programs

message(STATUS "Searching for IBVERBS library")

find_library(
  IBVERBS_LIBRARIES
  NAMES ibverbs
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  IBVERBS DEFAULT_MSG
  IBVERBS_LIBRARIES
)

mark_as_advanced(
  IBVERBS_LIBRARIES
)

if (IBVERBS_FOUND)
  message(STATUS "IBVERBS includes:  " ${IBVERBS_INCLUDE_DIRS})
  message(STATUS "IBVERBS libraries: " ${IBVERBS_LIBRARIES})
endif()