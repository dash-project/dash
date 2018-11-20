
##
# Query the current git HEAD (short) hash, if available
##
execute_process(COMMAND git rev-parse --short HEAD
                TIMEOUT 10
                RESULT_VARIABLE git_res
                OUTPUT_VARIABLE git_out
                ERROR_VARIABLE  git_err
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

if (git_res EQUAL 0)
  #remove newline
  string(REPLACE "\n" "" git_out ${git_out})
  message (STATUS "GIT Commit: ${git_out}")
  set (DASH_GIT_COMMIT "${git_out}")
  set (DASH_HAVE_GIT_COMMIT true)
endif()
