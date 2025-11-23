# Jikes Mini Runtime Library
# Minimal Java runtime classes for testing the Jikes compiler
# These are stub implementations, not a complete Java runtime

set(RUNTIME_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUNTIME_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(RUNTIME_JAR "${RUNTIME_OUTPUT_DIR}/runtime.jar")

# Find all runtime source files
file(GLOB_RECURSE RUNTIME_SOURCES "${RUNTIME_SRC_DIR}/*.java")

# Compile and package runtime as JAR
add_custom_command(
    OUTPUT ${RUNTIME_JAR}
    COMMAND $<TARGET_FILE:jikes> -source 1.5 -sourcepath "${RUNTIME_SRC_DIR}" -d "${RUNTIME_OUTPUT_DIR}/classes" ${RUNTIME_SOURCES}
    COMMAND jar cf "${RUNTIME_JAR}" -C "${RUNTIME_OUTPUT_DIR}/classes" java
    DEPENDS jikes ${RUNTIME_SOURCES}
    COMMENT "Building Jikes runtime JAR"
    VERBATIM
)

add_custom_target(runtime ALL
    DEPENDS ${RUNTIME_JAR}
)
