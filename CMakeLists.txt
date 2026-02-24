build_lib(
  LIBNAME orbitshield
  SOURCE_FILES
    model/satellite.cc
  HEADER_FILES
    model/satellite.h
  LIBRARIES_TO_LINK ${libcore} ${libnetwork}
  TEST_SOURCES
    test/test-satellite.cc
)
