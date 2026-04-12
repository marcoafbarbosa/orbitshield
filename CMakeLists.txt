
include(FetchContent)

# Download and setup the perturb library
FetchContent_Declare(
    perturb
    GIT_REPOSITORY "https://github.com/gunvirranu/perturb"
    GIT_TAG 128291c02675dc63ced11358129aa1384aec3545  # Master 2024/09/09
)
FetchContent_MakeAvailable(perturb)

target_compile_options(perturb PRIVATE -Wno-unused-variable)


build_lib(
  LIBNAME orbitshield
  SOURCE_FILES
    model/constellation.cc
    model/isl-propagation-delay-model.cc
    model/orbitshield-utils.cc
    model/satellite-link.cc
    model/satellite-mobility-model.cc
    model/satellite-net-device.cc
    model/satellite.cc
  HEADER_FILES
    model/constellation.h
    model/isl-propagation-delay-model.h
    model/orbitshield-utils.h
    model/satellite-link.h
    model/satellite-mobility-model.h
    model/satellite-net-device.h
    model/satellite.h
  LIBRARIES_TO_LINK ${libcore} ${libnetwork} ${libmobility} ${libpropagation} perturb
  TEST_SOURCES
    test/test-constellation.cc
    test/test-isl.cc
    test/test-satellite.cc
    test/test-orbitshield.cc
)

find_package(Boost COMPONENTS program_options QUIET)
set(orbitshield_isl_visualizer_libraries ${liborbitshield})
set(orbitshield_isl_visualizer_definitions)
if(Boost_PROGRAM_OPTIONS_FOUND)
  list(APPEND orbitshield_isl_visualizer_libraries Boost::program_options)
  list(APPEND orbitshield_isl_visualizer_definitions HAVE_BOOST_PROGRAM_OPTIONS)
else()
  message(STATUS "Boost.Program_options not found; isl-visualizer will use its fallback parser")
endif()

build_exec(
  EXECNAME orbitshield-isl-visualizer
  SOURCE_FILES
    tools/isl-visualizer.cc
  LIBRARIES_TO_LINK ${orbitshield_isl_visualizer_libraries}
  DEFINITIONS ${orbitshield_isl_visualizer_definitions}
  EXECUTABLE_DIRECTORY_PATH
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/contrib/orbitshield/
  INSTALL_DIRECTORY_PATH ${CMAKE_INSTALL_LIBEXECDIR}/orbitshield
  STANDALONE
)

