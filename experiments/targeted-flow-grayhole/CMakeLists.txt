build_exec(
  EXECNAME orbitshield-targeted-flow-grayhole
  SOURCE_FILES
    targeted-flow-grayhole.cc
  LIBRARIES_TO_LINK ${liborbitshield}
  EXECUTABLE_DIRECTORY_PATH
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/contrib/orbitshield/
  INSTALL_DIRECTORY_PATH ${CMAKE_INSTALL_LIBEXECDIR}/orbitshield
)