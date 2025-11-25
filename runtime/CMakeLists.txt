# Jopa Mini Runtime Library
# Minimal Java runtime classes for testing the Jopa compiler
# These are stub implementations, not a complete Java runtime

set(RUNTIME_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUNTIME_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(RUNTIME_JAR "${RUNTIME_OUTPUT_DIR}/runtime.jar")

# Find all runtime source files
file(GLOB_RECURSE RUNTIME_SOURCES "${RUNTIME_SRC_DIR}/*.java")

# Compile and package runtime as JAR
# Note: ASAN_OPTIONS=detect_leaks=0 disables leak detection since memory leaks
# in a compiler are not critical (OS reclaims memory on exit anyway)
add_custom_command(
    OUTPUT ${RUNTIME_JAR}
    COMMAND ${CMAKE_COMMAND} -E env "ASAN_OPTIONS=detect_leaks=0" $<TARGET_FILE:jopa> -source 1.5 -sourcepath "${RUNTIME_SRC_DIR}" -d "${RUNTIME_OUTPUT_DIR}/classes" ${RUNTIME_SOURCES}
    COMMAND jar cf "${RUNTIME_JAR}" -C "${RUNTIME_OUTPUT_DIR}/classes" java
    DEPENDS jopa ${RUNTIME_SOURCES}
    COMMENT "Building Jopa runtime JAR"
    VERBATIM
)

add_custom_target(runtime ALL
    DEPENDS ${RUNTIME_JAR}
)
