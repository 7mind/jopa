# Grammar generation using jikespg
#
# jikespg is built from source as part of the main build.
# Grammar files are always regenerated from java.g.

set(GRAMMAR_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/java.g")
set(GRAMMAR_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(POSTPROCESS_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/postprocess.py")

# Temporary directory for raw jikespg output
set(JIKESPG_TEMP_DIR "${GRAMMAR_OUTPUT_DIR}/jikespg-temp")

# Create temp directory at configure time
file(MAKE_DIRECTORY "${JIKESPG_TEMP_DIR}")

# Raw output files from jikespg (in temp directory)
set(JIKESPG_RAW_OUTPUTS
    ${JIKESPG_TEMP_DIR}/javadcl.h
    ${JIKESPG_TEMP_DIR}/javadef.h
    ${JIKESPG_TEMP_DIR}/javaprs.h
    ${JIKESPG_TEMP_DIR}/javasym.h
)

# Processed output files (in final output directory)
set(GRAMMAR_PROCESSED_OUTPUTS
    ${GRAMMAR_OUTPUT_DIR}/javadcl.h
    ${GRAMMAR_OUTPUT_DIR}/javadef.h
    ${GRAMMAR_OUTPUT_DIR}/javaprs.h
    ${GRAMMAR_OUTPUT_DIR}/javasym.h
)

# Custom command to run jikespg (outputs to temp directory)
# We copy java.g to the temp directory first since jikespg outputs to its working directory
add_custom_command(
    OUTPUT ${JIKESPG_RAW_OUTPUTS}
    COMMAND ${CMAKE_COMMAND} -E copy ${GRAMMAR_SOURCE} ${JIKESPG_TEMP_DIR}/java.g
    COMMAND $<TARGET_FILE:jikespg> java
    WORKING_DIRECTORY ${JIKESPG_TEMP_DIR}
    DEPENDS ${GRAMMAR_SOURCE} jikespg
    COMMENT "Generating parser tables from java.g using jikespg"
    VERBATIM
)

# Custom command to post-process the generated files (outputs to final directory)
add_custom_command(
    OUTPUT ${GRAMMAR_PROCESSED_OUTPUTS}
    COMMAND python3 ${POSTPROCESS_SCRIPT} ${JIKESPG_TEMP_DIR} ${GRAMMAR_OUTPUT_DIR} java
    WORKING_DIRECTORY ${JIKESPG_TEMP_DIR}
    DEPENDS ${JIKESPG_RAW_OUTPUTS} ${POSTPROCESS_SCRIPT}
    COMMENT "Post-processing generated parser headers (adding namespace)"
    VERBATIM
)

# Custom target for grammar generation
add_custom_target(generate_parser
    DEPENDS ${GRAMMAR_PROCESSED_OUTPUTS}
    COMMENT "Parser generation complete"
)
