project(project_dash_dart_if)

# Get DART interface version number:
set(DASH_DART_IF_INCLUDE_DIR ${DASH_DART_IF_INCLUDE_DIR} PARENT_SCOPE)

## Installation

# Headers
install(
  DIRECTORY ${DASH_DART_IF_INCLUDE_DIR} DESTINATION include
  FILES_MATCHING PATTERN "*.h"
)
