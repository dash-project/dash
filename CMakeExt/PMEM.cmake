# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

# - Find Required PMEM libraries (libvmem, libpmem, libpmemobj)
# This module defines
#  PMEM_FOUND
#  PMEM_INCLUDE_DIRS, directory containing headers
#  PMEM_LIBRARIES, directory containing libraries

if (NOT PMEM_PREFIX)
  find_path(
    PMEM_PREFIX
    NAMES include/libpmem.h
  )
endif()

if (NOT PMEM_PREFIX)
  set(PMEM_PREFIX "/usr/")
endif()

message(STATUS "Searching for PMEM library in path " ${PMEM_PREFIX})

set(PMEM_SEARCH_LIB_PATH
  ${PMEM_PREFIX}/lib
)
set(PMEM_SEARCH_HEADER_PATH
  ${PMEM_PREFIX}/include
)

find_path(
  PMEM_INCLUDE_DIRS
  NAMES libpmemobj.h
  PATHS ${PMEM_SEARCH_HEADER_PATH}
  # make sure we don't accidentally pick up a different version
  NO_DEFAULT_PATH
)

find_library(
  PMEM_LIBRARIES
  NAMES pmemobj
  PATHS ${PMEM_SEARCH_LIB_PATH} 
  NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  PMEM DEFAULT_MSG
  PMEM_LIBRARIES
  PMEM_INCLUDE_DIRS
)

if (PMEM_FOUND)
  if (NOT PMEM_FIND_QUIETLY)
    message(STATUS "PMEM includes:  " ${PMEM_INCLUDE_DIRS})
    message(STATUS "PMEM libraries: " ${PMEM_LIBRARIES})
  endif()
else()
  if (NOT PMEM_FIND_QUIETLY)
    set(PMEM_ERR_MSG "Could not find the pmem libraries. Looked for headers")
    set(PMEM_ERR_MSG "${PMEM_ERR_MSG} in ${PMEM_SEARCH_HEADER_PATH}, and for libs")
    set(PMEM_ERR_MSG "${PMEM_ERR_MSG} in ${PMEM_SEARCH_LIB_PATH}")
  endif()
endif()

mark_as_advanced(
  PMEM_LIBRARIES
  PMEM_INCLUDE_DIRS
)
