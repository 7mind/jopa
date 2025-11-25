# Java Feature Tests for Jopa Compiler
# Tests both compilation and runtime execution with real JVM

enable_testing()

# Option to enable JDK parser tests
option(JOPA_ENABLE_JDK_PARSER_TESTS "Enable JDK language tools parser tests" OFF)
option(JOPA_PARSER_TESTS_JDK7 "Enable JDK7 parser tests" ON)
option(JOPA_PARSER_TESTS_JDK8 "Enable JDK8 parser tests" ON)

# Option to enable/disable JVM validation tests
option(JOPA_ENABLE_JVM_TESTS "Enable runtime tests using real JVM (requires Java)" ON)

# Find Java for runtime tests (only if JVM tests are enabled)
if(JOPA_ENABLE_JVM_TESTS)
    find_package(Java COMPONENTS Runtime Development REQUIRED)

    # Locate javap manually if not found
    if(NOT Java_JAVAP_EXECUTABLE)
        get_filename_component(JAVA_BIN_DIR ${Java_JAVA_EXECUTABLE} DIRECTORY)
        find_program(Java_JAVAP_EXECUTABLE javap HINTS ${JAVA_BIN_DIR})
    endif()
endif()

set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUNTIME_SRC_DIR "${CMAKE_SOURCE_DIR}/runtime")
set(RUNTIME_JAR "${CMAKE_BINARY_DIR}/runtime/runtime.jar")
set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Helper function to add a compile-only test
# Note: compile-only tests may generate multiple classes, so we don't validate
# them with javap (which requires knowing the exact class name)
function(add_jopa_compile_test test_name source_file)
    add_test(
        NAME "compile_${test_name}"
        COMMAND $<TARGET_FILE:jopa> -source 1.5
                -sourcepath "${TEST_DIR}"
                -classpath "${RUNTIME_JAR}"
                -d "${OUTPUT_DIR}"
                "${source_file}"
    )
    set_tests_properties("compile_${test_name}" PROPERTIES
        LABELS "compile"
    )
endfunction()

# Helper function to add a compile+run test
function(add_jopa_run_test test_name source_file main_class)
    # Compilation test
    add_test(
        NAME "compile_${test_name}"
        COMMAND $<TARGET_FILE:jopa> -source 1.5
                -sourcepath "${TEST_DIR}"
                -classpath "${RUNTIME_JAR}"
                -d "${OUTPUT_DIR}"
                "${source_file}"
    )
    set_tests_properties("compile_${test_name}" PROPERTIES
        LABELS "compile"
    )

    # Javap validation and runtime test (only if JVM tests are enabled)
    if(JOPA_ENABLE_JVM_TESTS)
        # Validate bytecode with javap
        add_test(
            NAME "validate_${test_name}"
            COMMAND ${Java_JAVAP_EXECUTABLE} -constants "${OUTPUT_DIR}/${main_class}.class"
        )
        set_tests_properties("validate_${test_name}" PROPERTIES
            LABELS "validate"
            DEPENDS "compile_${test_name}"
        )

        # Runtime test
        add_test(
            NAME "run_${test_name}"
            COMMAND ${Java_JAVA_EXECUTABLE} -cp "${OUTPUT_DIR}:${RUNTIME_JAR}" "${main_class}"
        )
        set_tests_properties("run_${test_name}" PROPERTIES
            LABELS "run"
            DEPENDS "validate_${test_name}"
        )
    endif()
endfunction()

# Enum Tests
add_jopa_compile_test(Color "${TEST_DIR}/enums/Color.java")
add_jopa_run_test(ColorTest "${TEST_DIR}/enums/ColorTest.java" "ColorTest")
add_test(
    NAME "compile_EnumMethodsTest"
    COMMAND $<TARGET_FILE:jopa> -source 1.5
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${OUTPUT_DIR}"
            "${TEST_DIR}/enums/Color.java"
            "${TEST_DIR}/enums/EnumMethodsTest.java"
)
set_tests_properties("compile_EnumMethodsTest" PROPERTIES LABELS "compile")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "run_EnumMethodsTest"
        COMMAND ${Java_JAVA_EXECUTABLE} -cp "${OUTPUT_DIR}:${RUNTIME_JAR}" "EnumMethodsTest"
    )
    set_tests_properties("run_EnumMethodsTest" PROPERTIES
        LABELS "run"
        DEPENDS "compile_EnumMethodsTest"
    )
endif()

# Varargs Tests
add_jopa_run_test(VarargsTest "${TEST_DIR}/varargs/VarargsTest.java" "VarargsTest")
add_jopa_compile_test(VarargsImplicitTest "${TEST_DIR}/varargs/VarargsImplicitTest.java")

# Enhanced For-Loop Tests
add_jopa_run_test(ForEachTest "${TEST_DIR}/foreach/ForEachTest.java" "ForEachTest")
add_jopa_compile_test(ForEachArrayTest "${TEST_DIR}/foreach/ForEachArrayTest.java")
add_jopa_run_test(ForEachIterableTest "${TEST_DIR}/foreach/ForEachIterableTest.java" "ForEachIterableTest")

# Generics Tests
add_jopa_compile_test(GenericBox "${TEST_DIR}/generics/GenericBox.java")
add_jopa_run_test(GenericTest "${TEST_DIR}/generics/GenericTest.java" "GenericTest")
add_jopa_compile_test(SimpleGeneric "${TEST_DIR}/generics/SimpleGeneric.java")
add_jopa_compile_test(TwoTypeParams "${TEST_DIR}/generics/TwoTypeParams.java")
add_jopa_compile_test(BoundedGeneric "${TEST_DIR}/generics/BoundedGeneric.java")
add_jopa_compile_test(Wildcards "${TEST_DIR}/generics/Wildcards.java")
add_jopa_compile_test(GenericMethods "${TEST_DIR}/generics/GenericMethods.java")
add_jopa_compile_test(CovariantReturnTest "${TEST_DIR}/generics/CovariantReturnTest.java")
add_jopa_compile_test(BridgeTest "${TEST_DIR}/generics/BridgeTest.java")
add_jopa_compile_test(SimpleSameFileTest "${TEST_DIR}/generics/SimpleSameFileTest.java")
add_jopa_compile_test(TypeSubstitutionTest "${TEST_DIR}/generics/TypeSubstitutionTest.java")
add_jopa_compile_test(FieldAccessTest "${TEST_DIR}/generics/FieldAccessTest.java")
add_jopa_run_test(ArrayCovarianceTest "${TEST_DIR}/generics/ArrayCovarianceTest.java" "ArrayCovarianceTest")
add_jopa_run_test(GenericVarianceTest "${TEST_DIR}/generics/GenericVarianceTest.java" "GenericVarianceTest")

# Autoboxing/Unboxing Tests
add_jopa_run_test(BasicBoxingTest "${TEST_DIR}/autoboxing/BasicBoxingTest.java" "BasicBoxingTest")
add_jopa_run_test(AutoboxingTest "${TEST_DIR}/autoboxing/AutoboxingTest.java" "AutoboxingTest")
add_jopa_run_test(MethodBoxingSimple "${TEST_DIR}/autoboxing/MethodBoxingSimple.java" "MethodBoxingSimple")
add_jopa_run_test(ReturnBoxingSimple "${TEST_DIR}/autoboxing/ReturnBoxingSimple.java" "ReturnBoxingSimple")
add_jopa_run_test(ArithmeticBoxingSimple "${TEST_DIR}/autoboxing/ArithmeticBoxingSimple.java" "ArithmeticBoxingSimple")

# Static Import Tests
add_test(
    NAME "compile_MathUtils"
    COMMAND $<TARGET_FILE:jopa> -source 1.5
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${OUTPUT_DIR}"
            "${TEST_DIR}/util/MathUtils.java"
)
set_tests_properties("compile_MathUtils" PROPERTIES LABELS "compile")

add_jopa_run_test(StaticFieldOnlyTest "${TEST_DIR}/static-imports/StaticFieldOnlyTest.java" "StaticFieldOnlyTest")
add_jopa_run_test(StaticMethodImportTest "${TEST_DIR}/static-imports/StaticMethodImportTest.java" "StaticMethodImportTest")
add_jopa_run_test(StaticWildcardImportTest "${TEST_DIR}/static-imports/StaticWildcardImportTest.java" "StaticWildcardImportTest")

# Annotation Tests
add_jopa_run_test(AnnotationTest "${TEST_DIR}/annotations/AnnotationTest.java" "AnnotationTest")
add_jopa_run_test(MarkerAnnotationTest "${TEST_DIR}/annotations/MarkerAnnotationTest.java" "MarkerAnnotationTest")
add_jopa_run_test(CustomAnnotationTest "${TEST_DIR}/annotations/CustomAnnotationTest.java" "CustomAnnotationTest")
add_jopa_run_test(SuppressWarningsTest "${TEST_DIR}/annotations/SuppressWarningsTest.java" "SuppressWarningsTest")

# Java 6 Tests (bytecode version 50.0 with parameter names)
add_test(
    NAME "compile_SimpleJava6Test"
    COMMAND $<TARGET_FILE:jopa> -source 1.5 -target 1.6 -g
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${OUTPUT_DIR}"
            "${TEST_DIR}/java6/SimpleJava6Test.java"
)
set_tests_properties("compile_SimpleJava6Test" PROPERTIES LABELS "compile")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "run_SimpleJava6Test"
        COMMAND ${Java_JAVA_EXECUTABLE} -cp "${OUTPUT_DIR}:${RUNTIME_JAR}" "SimpleJava6Test"
    )
    set_tests_properties("run_SimpleJava6Test" PROPERTIES
        LABELS "run"
        DEPENDS "compile_SimpleJava6Test"
    )
endif()

# Reflection Tests (compile with our runtime, run with system JVM for real reflection)
add_test(
    NAME "compile_ReflectionBasicTest"
    COMMAND $<TARGET_FILE:jopa> -source 1.5 -target 1.6 -g
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${OUTPUT_DIR}"
            "${TEST_DIR}/java6/ReflectionBasicTest.java"
)
set_tests_properties("compile_ReflectionBasicTest" PROPERTIES LABELS "compile")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "run_ReflectionBasicTest"
        COMMAND ${Java_JAVA_EXECUTABLE} -cp "${OUTPUT_DIR}" "ReflectionBasicTest"
    )
    set_tests_properties("run_ReflectionBasicTest" PROPERTIES
        LABELS "run"
        DEPENDS "compile_ReflectionBasicTest"
    )
endif()

# Java 7 Tests (bytecode version 51.0 with diamond operator)
add_test(
    NAME "compile_DiamondTest"
    COMMAND $<TARGET_FILE:jopa> -source 1.7 -target 1.7
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${OUTPUT_DIR}"
            "${TEST_DIR}/java7/DiamondTest.java"
)
set_tests_properties("compile_DiamondTest" PROPERTIES LABELS "compile;java7")
if(JOPA_ENABLE_JVM_TESTS)
    # Use -noverify because StackMapTable is not yet implemented for Java 7+ bytecode
    add_test(
        NAME "run_DiamondTest"
        COMMAND ${Java_JAVA_EXECUTABLE} -noverify -cp "${OUTPUT_DIR}:${RUNTIME_JAR}" "DiamondTest"
    )
    set_tests_properties("run_DiamondTest" PROPERTIES
        LABELS "run;java7"
        DEPENDS "compile_DiamondTest"
    )
endif()

# Java 8 Tests (bytecode version 52.0 with default methods)
add_test(
    NAME "compile_DefaultMethodTest"
    COMMAND $<TARGET_FILE:jopa> -source 1.8 -target 1.8
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${OUTPUT_DIR}"
            "${TEST_DIR}/java8/DefaultMethodTest.java"
)
set_tests_properties("compile_DefaultMethodTest" PROPERTIES LABELS "compile;java8")
if(JOPA_ENABLE_JVM_TESTS)
    # Use -noverify because StackMapTable is not yet implemented for Java 8 bytecode
    add_test(
        NAME "run_DefaultMethodTest"
        COMMAND ${Java_JAVA_EXECUTABLE} -noverify -cp "${OUTPUT_DIR}:${RUNTIME_JAR}" "DefaultMethodTest"
    )
    set_tests_properties("run_DefaultMethodTest" PROPERTIES
        LABELS "run;java8"
        DEPENDS "compile_DefaultMethodTest"
    )
endif()

# ============================================================================
# JDK Language Tools Parser Tests
# ============================================================================

if(JOPA_ENABLE_JDK_PARSER_TESTS)
    set(PARSER_TEST_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/parser_tests")
    file(MAKE_DIRECTORY "${PARSER_TEST_OUTPUT_DIR}")

    # Function to add parser tests for a JDK version
    function(add_jdk_parser_tests jdk_version jdk_assets_dir)
        if(NOT EXISTS "${jdk_assets_dir}")
            message(WARNING "JDK${jdk_version} assets not found at ${jdk_assets_dir}")
            return()
        endif()

        # Find all .java files
        file(GLOB_RECURSE JAVA_FILES "${jdk_assets_dir}/*.java")

        # Find all .out files (negative test markers)
        file(GLOB_RECURSE OUT_FILES "${jdk_assets_dir}/*.out")

        # Create a list of base paths for negative tests
        set(NEGATIVE_TESTS "")
        foreach(out_file ${OUT_FILES})
            get_filename_component(base_path "${out_file}" DIRECTORY)
            get_filename_component(base_name "${out_file}" NAME_WE)
            list(APPEND NEGATIVE_TESTS "${base_path}/${base_name}")
        endforeach()

        set(test_count 0)
        set(skipped_count 0)
        foreach(java_file ${JAVA_FILES})
            # Get base path without extension
            get_filename_component(file_dir "${java_file}" DIRECTORY)
            get_filename_component(file_name_we "${java_file}" NAME_WE)
            set(base_path "${file_dir}/${file_name_we}")

            # Skip if this is a negative test (has matching .out file)
            list(FIND NEGATIVE_TESTS "${base_path}" is_negative)
            if(NOT is_negative EQUAL -1)
                math(EXPR skipped_count "${skipped_count} + 1")
                continue()
            endif()

            # Skip files containing @compile/fail (expected to fail compilation)
            file(READ "${java_file}" file_content)
            string(FIND "${file_content}" "@compile/fail" compile_fail_pos)
            if(NOT compile_fail_pos EQUAL -1)
                math(EXPR skipped_count "${skipped_count} + 1")
                continue()
            endif()

            # Skip diagnostic examples (key: compiler.err/warn - intentionally invalid/warning code)
            string(FIND "${file_content}" "// key: compiler.err" diag_err_pos)
            string(FIND "${file_content}" "// key: compiler.warn" diag_warn_pos)
            if(NOT diag_err_pos EQUAL -1 OR NOT diag_warn_pos EQUAL -1)
                math(EXPR skipped_count "${skipped_count} + 1")
                continue()
            endif()

            # Skip files marked as invalid
            string(FIND "${file_content}" "this class is invalid" invalid_marker_pos)
            string(FIND "${file_content}" "// invalid" invalid_comment_pos)
            if(NOT invalid_marker_pos EQUAL -1 OR NOT invalid_comment_pos EQUAL -1)
                math(EXPR skipped_count "${skipped_count} + 1")
                continue()
            endif()

            # Skip files in "bad" directories or named BadSource/Error/ParseErrors
            string(FIND "${java_file}" "/bad/" bad_dir_pos)
            string(FIND "${java_file}" "BadSource.java" bad_source_pos)
            string(FIND "${java_file}" "ParseErrors.java" parse_errors_pos)
            if(NOT bad_dir_pos EQUAL -1 OR NOT bad_source_pos EQUAL -1 OR NOT parse_errors_pos EQUAL -1)
                math(EXPR skipped_count "${skipped_count} + 1")
                continue()
            endif()

            # Create relative path for unique test name
            file(RELATIVE_PATH rel_path "${jdk_assets_dir}" "${java_file}")
            string(REPLACE "/" "_" test_name "${rel_path}")
            string(REPLACE ".java" "" test_name "${test_name}")

            # Create output file for this test
            set(output_file "${PARSER_TEST_OUTPUT_DIR}/jdk${jdk_version}_${test_name}.result")

            # Add the parse test with appropriate source level
            add_test(
                NAME "parse_jdk${jdk_version}_${test_name}"
                COMMAND $<TARGET_FILE:jopa> -source "1.${jdk_version}" --parse-only "${output_file}" "${java_file}"
            )
            set_tests_properties("parse_jdk${jdk_version}_${test_name}" PROPERTIES
                LABELS "parser;jdk${jdk_version}"
            )

            math(EXPR test_count "${test_count} + 1")
        endforeach()

        message(STATUS "Added ${test_count} JDK${jdk_version} parser tests (skipped ${skipped_count} negative tests)")
    endfunction()

    # Add JDK7 parser tests
    if(JOPA_PARSER_TESTS_JDK7)
        add_jdk_parser_tests(7 "${CMAKE_SOURCE_DIR}/assets/jdk7u-langtools/test")
    endif()

    # Add JDK8 parser tests
    if(JOPA_PARSER_TESTS_JDK8)
        add_jdk_parser_tests(8 "${CMAKE_SOURCE_DIR}/assets/jdk8u_langtools/test")
    endif()
endif()
