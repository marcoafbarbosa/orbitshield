build_lib_example(
  NAME orbitshield-basic-example
  SOURCE_FILES orbitshield-basic-example.cc
  LIBRARIES_TO_LINK ${liborbitshield}
)

build_lib_example(
  NAME orbitshield-load-from-tle
  SOURCE_FILES orbitshield-load-from-tle.cc
  LIBRARIES_TO_LINK ${liborbitshield}
)

build_lib_example(
  NAME orbitshield-load-from-ring
  SOURCE_FILES orbitshield-load-from-ring.cc
  LIBRARIES_TO_LINK ${liborbitshield}
)
