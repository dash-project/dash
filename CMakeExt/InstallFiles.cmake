
function(DeployBinary
         bin
         )         
    add_custom_command(
      TARGET ${bin}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${bin}> ${CMAKE_BINARY_DIR}/bin/
    )
endfunction(DeployBinary)

function(DeployLibrary
         lib
         )         
    add_custom_command(
      TARGET ${lib}
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${lib}> ${CMAKE_BINARY_DIR}/lib/
    )
endfunction(DeployLibrary)
