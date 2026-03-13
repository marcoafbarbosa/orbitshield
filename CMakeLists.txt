
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
    model/satellite.cc
    model/constellation.cc
  HEADER_FILES
    model/satellite.h
    model/constellation.h
  LIBRARIES_TO_LINK ${libcore} ${libnetwork} perturb
  TEST_SOURCES
    test/test-satellite.cc
    test/test-constellation.cc
)
