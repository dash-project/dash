function (CreateDoxygenTarget)
  find_package(Doxygen)
  if (NOT DOXYGEN_FOUND)
    message(WARNING "!! Doxygen not found, not building documentation")
  else()
    set(doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/doc/config/Doxyfile.in)
    set(doxyfile ${CMAKE_CURRENT_BINARY_DIR}/doc/config/Doxyfile)
    set(DASH_VERSIONED_PROJECT_NAME ${DASH_VERSIONED_PROJECT_NAME}
        PARENT_SCOPE)

    configure_file(${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_target(doc
      COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM
    )
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc
            DESTINATION share/doc/${DASH_VERSIONED_PROJECT_NAME})

#   configure_file(doc/config/Doxyfile.in
#                  ${PROJECT_BINARY_DIR}/Doxyfile
#                  @ONLY IMMEDIATE)
#   if (TARGET doxygen)
#     message("-- Target doxygen added")
#   else()
#     add_custom_target (
#       doxygen 
#       COMMAND ${DOXYGEN_EXECUTABLE} ${PROJECT_BINARY_DIR}/Doxyfile
#       SOURCES ${PROJECT_BINARY_DIR}/Doxyfile)
#       install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/
#               DESTINATION doc FILES_MATCHING 
#                 PATTERN "*.*"
#                 PATTERN "CMakeLists.txt" EXCLUDE)
#   endif()
  endif()
endfunction()
