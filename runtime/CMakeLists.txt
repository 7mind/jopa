# Jopa Mini Runtime Library
# Minimal Java runtime classes for testing the Jopa compiler
# These are stub implementations, not a complete Java runtime

set(RUNTIME_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUNTIME_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(RUNTIME_JAR "${RUNTIME_OUTPUT_DIR}/jopa-stub-rt.jar")

# Find all runtime source files
file(GLOB_RECURSE RUNTIME_SOURCES "${RUNTIME_SRC_DIR}/*.java")

# Compile and package runtime as JAR (using zip instead of jar tool)
add_custom_command(
    OUTPUT ${RUNTIME_JAR}
    COMMAND $<TARGET_FILE:jopa> --nowarn:unchecked -source 1.5 -sourcepath "${RUNTIME_SRC_DIR}" -d "${RUNTIME_OUTPUT_DIR}/classes" ${RUNTIME_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RUNTIME_OUTPUT_DIR}/classes/META-INF"
    COMMAND sh -c "echo 'Manifest-Version: 1.0' > '${RUNTIME_OUTPUT_DIR}/classes/META-INF/MANIFEST.MF'"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${RUNTIME_JAR}"
    COMMAND sh -c "cd '${RUNTIME_OUTPUT_DIR}/classes' && zip -qr '${RUNTIME_JAR}' ."
    DEPENDS jopa ${RUNTIME_SOURCES}
    COMMENT "Building Jopa runtime JAR"
    VERBATIM
)

add_custom_target(jopa-stub-rt ALL
    DEPENDS ${RUNTIME_JAR}
)
