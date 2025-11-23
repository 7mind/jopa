# Jikes Mini Runtime Library
# Minimal Java runtime classes for testing the Jikes compiler
# These are stub implementations, not a complete Java runtime

set(RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUNTIME_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# List of runtime source files
set(RUNTIME_SOURCES
    "${RUNTIME_DIR}/java/util/Iterator.java"
    "${RUNTIME_DIR}/java/lang/Iterable.java"
    "${RUNTIME_DIR}/java/util/ArrayList.java"
)

# Compile runtime classes
add_custom_command(
    OUTPUT "${RUNTIME_DIR}/java/util/Iterator.class"
           "${RUNTIME_DIR}/java/lang/Iterable.class"
           "${RUNTIME_DIR}/java/util/ArrayList.class"
    COMMAND $<TARGET_FILE:jikes> -source 1.5 -sourcepath "${RUNTIME_DIR}" -d "${RUNTIME_DIR}" ${RUNTIME_SOURCES}
    DEPENDS jikes ${RUNTIME_SOURCES}
    COMMENT "Compiling Jikes mini runtime library"
)

add_custom_target(runtime ALL
    DEPENDS "${RUNTIME_DIR}/java/util/Iterator.class"
            "${RUNTIME_DIR}/java/lang/Iterable.class"
            "${RUNTIME_DIR}/java/util/ArrayList.class"
)
