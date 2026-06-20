build_exec(
  EXECNAME orbitshield-targeted-flow-grayhole
  SOURCE_FILES
    targeted-flow-grayhole.cc
    targeted-flow-grayhole-config.cc
    targeted-flow-grayhole-detector.cc
    targeted-flow-grayhole-runner.cc
    targeted-flow-grayhole-telemetry.cc
  LIBRARIES_TO_LINK ${liborbitshield} yaml-cpp
  EXECUTABLE_DIRECTORY_PATH
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/contrib/orbitshield/
  INSTALL_DIRECTORY_PATH ${CMAKE_INSTALL_LIBEXECDIR}/orbitshield
)