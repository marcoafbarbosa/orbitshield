
include(FetchContent)

# Download and setup the perturb library
FetchContent_Declare(
    perturb
    GIT_REPOSITORY "https://github.com/gunvirranu/perturb"
    GIT_TAG 5723e50a54d438876b7a02e02a326e30b51ead4c  # Release v1.0.0
)
FetchContent_MakeAvailable(perturb)


build_lib(
  LIBNAME orbitshield
  SOURCE_FILES
    model/constellation.cc
    model/isl-propagation-delay-model.cc
    model/satellite-link.cc
    model/satellite-mobility-model.cc
    model/satellite-net-device.cc
    model/satellite.cc
  HEADER_FILES
    model/constellation.h
    model/isl-propagation-delay-model.h
    model/satellite-link.h
    model/satellite-mobility-model.h
    model/satellite-net-device.h
    model/satellite.h
  LIBRARIES_TO_LINK ${libcore} ${libnetwork} ${libmobility} ${libpropagation} perturb
  TEST_SOURCES
    test/test-constellation.cc
    test/test-isl.cc
    test/test-satellite.cc
)
