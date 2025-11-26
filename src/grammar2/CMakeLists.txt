# Grammar generation using jikespg

set(JIKESPG_EXECUTABLE "${CMAKE_SOURCE_DIR}/tools/jikespg/src/jikespg")
set(GRAMMAR_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/java.g")
set(GRAMMAR_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(GRAMMAR_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(POSTPROCESS_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/postprocess.py")

# Raw output files from jikespg (in source directory)
set(JIKESPG_RAW_OUTPUTS
    ${GRAMMAR_SRC_DIR}/javadcl.h
    ${GRAMMAR_SRC_DIR}/javadef.h
    ${GRAMMAR_SRC_DIR}/javaprs.h
    ${GRAMMAR_SRC_DIR}/javasym.h
)

# Processed output files (in build directory)
set(GRAMMAR_PROCESSED_OUTPUTS
    ${GRAMMAR_OUTPUT_DIR}/javadcl.h
    ${GRAMMAR_OUTPUT_DIR}/javadef.h
    ${GRAMMAR_OUTPUT_DIR}/javaprs.h
    ${GRAMMAR_OUTPUT_DIR}/javasym.h
)

# Check if jikespg exists
if(NOT EXISTS ${JIKESPG_EXECUTABLE})
    message(WARNING "jikespg not found at ${JIKESPG_EXECUTABLE}. Grammar will not be regenerated.")
    message(WARNING "Build jikespg first: cd tools/jikespg/src && make")
else()
    # Custom command to run jikespg (outputs to source directory)
    add_custom_command(
        OUTPUT ${JIKESPG_RAW_OUTPUTS}
        COMMAND ${JIKESPG_EXECUTABLE} java
        WORKING_DIRECTORY ${GRAMMAR_SRC_DIR}
        DEPENDS ${GRAMMAR_SOURCE}
        COMMENT "Generating parser tables from java.g using jikespg"
        VERBATIM
    )

    # Custom command to post-process the generated files (outputs to build directory)
    add_custom_command(
        OUTPUT ${GRAMMAR_PROCESSED_OUTPUTS}
        COMMAND python3 ${POSTPROCESS_SCRIPT} ${GRAMMAR_SRC_DIR} ${GRAMMAR_OUTPUT_DIR} java
        WORKING_DIRECTORY ${GRAMMAR_SRC_DIR}
        DEPENDS ${JIKESPG_RAW_OUTPUTS} ${POSTPROCESS_SCRIPT}
        COMMENT "Post-processing generated parser headers (adding namespace)"
        VERBATIM
    )

    # Custom target for grammar generation
    add_custom_target(generate_parser
        DEPENDS ${GRAMMAR_PROCESSED_OUTPUTS}
        COMMENT "Parser generation complete"
    )
endif()

# Export the include directory for the processed headers
set(GRAMMAR2_INCLUDE_DIR ${GRAMMAR_OUTPUT_DIR} PARENT_SCOPE)
