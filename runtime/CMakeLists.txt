# Jikes Mini Runtime Library
# Minimal Java runtime classes for testing the Jikes compiler
# These are stub implementations, not a complete Java runtime

set(RUNTIME_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUNTIME_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Find all runtime source files
file(GLOB_RECURSE RUNTIME_SOURCES "${RUNTIME_SRC_DIR}/*.java")

# Generate list of expected .class files
set(RUNTIME_CLASSES "")
foreach(src ${RUNTIME_SOURCES})
    # Convert .java path to .class path
    string(REPLACE "${RUNTIME_SRC_DIR}" "${RUNTIME_OUTPUT_DIR}" class_path "${src}")
    string(REPLACE ".java" ".class" class_path "${class_path}")
    list(APPEND RUNTIME_CLASSES "${class_path}")
endforeach()

# Compile all runtime classes to build directory
add_custom_command(
    OUTPUT ${RUNTIME_CLASSES}
    COMMAND $<TARGET_FILE:jikes> -source 1.5 -sourcepath "${RUNTIME_SRC_DIR}" -d "${RUNTIME_OUTPUT_DIR}" ${RUNTIME_SOURCES}
    DEPENDS jikes ${RUNTIME_SOURCES}
    COMMENT "Compiling Jikes mini runtime library (${RUNTIME_SOURCES})"
    VERBATIM
)

add_custom_target(runtime ALL
    DEPENDS ${RUNTIME_CLASSES}
)
