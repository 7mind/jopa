# Java Feature Tests for Jopa Compiler
# Tests compilation and runtime execution with real JVM
#
# All tests use -source 1.7 (Java 7 language features).
# Bytecode target is controlled by JOPA_TARGET_VERSION (default: 1.5).
# To test the full matrix, run cmake with different target versions:
#   cmake -DJOPA_TARGET_VERSION=1.5 ..  # Java 5 bytecode (v49)
#   cmake -DJOPA_TARGET_VERSION=1.6 ..  # Java 6 bytecode (v50)
#   cmake -DJOPA_TARGET_VERSION=1.7 ..  # Java 7 bytecode (v51, needs StackMapTable)

enable_testing()

# ASAN/UBSAN environment for tests (disable leak detection, allow UB during build)
set(JOPA_TEST_ENVIRONMENT "ASAN_OPTIONS=detect_leaks=0" "UBSAN_OPTIONS=halt_on_error=0")


# Option to enable JDK parser tests
option(JOPA_ENABLE_JDK_PARSER_TESTS "Enable JDK language tools parser tests" OFF)
option(JOPA_PARSER_TESTS_JDK7 "Enable JDK7 parser tests" ON)
option(JOPA_PARSER_TESTS_JDK8 "Enable JDK8 parser tests" ON)

# Option to enable/disable JVM validation tests
option(JOPA_ENABLE_JVM_TESTS "Enable runtime tests using real JVM (requires Java)" ON)

# Option to use -noverify flag for JVM tests
# Required until StackMapTable generation is implemented for Java 7+ bytecode
option(JOPA_USE_NOVERIFY "Use -noverify flag for JVM runtime tests" OFF)

# Bytecode target version for tests (1.5, 1.6, or 1.7)
# 1.5 and 1.6 work without StackMapTable, 1.7 requires it
set(JOPA_TARGET_VERSION "1.5" CACHE STRING "Bytecode target version for tests (1.5, 1.6, 1.7)")
set_property(CACHE JOPA_TARGET_VERSION PROPERTY STRINGS "1.5" "1.6" "1.7")

# Find Java for runtime tests (only if JVM tests are enabled)
if(JOPA_ENABLE_JVM_TESTS)
    find_package(Java COMPONENTS Runtime Development REQUIRED)

    # Locate javap manually if not found
    if(NOT Java_JAVAP_EXECUTABLE)
        get_filename_component(JAVA_BIN_DIR ${Java_JAVA_EXECUTABLE} DIRECTORY)
        find_program(Java_JAVAP_EXECUTABLE javap HINTS ${JAVA_BIN_DIR})
    endif()

    set(TEST_JAVA_EXECUTABLE "${Java_JAVA_EXECUTABLE}")
    set(TEST_JAVA_BOOTCP_FLAGS "")
    set(JOPA_EXTRA_FLAGS "")
    
    # Set up JVM flags based on options
    if(JOPA_USE_NOVERIFY)
        set(JVM_TEST_FLAGS "-noverify")
    else()
        set(JVM_TEST_FLAGS "")
    endif()
endif()

set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUNTIME_SRC_DIR "${CMAKE_SOURCE_DIR}/runtime")
set(RUNTIME_JAR "${CMAKE_BINARY_DIR}/runtime/jopa-stub-rt.jar")
set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Helper function to add a compile+run test
# Usage: add_jopa_run_test(test_name source_file main_class [extra_sources...])
# Each test gets its own output directory to avoid class file collisions
function(add_jopa_run_test test_name source_file main_class)
    set(TEST_OUTPUT_DIR "${OUTPUT_DIR}/${test_name}")
    file(MAKE_DIRECTORY "${TEST_OUTPUT_DIR}")
    # Compilation test - compiles main source plus any extra sources
    add_test(
        NAME "compile_${test_name}"
        COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.7 -target ${JOPA_TARGET_VERSION}
                -sourcepath "${TEST_DIR}"
                -classpath "${RUNTIME_JAR}"
                -d "${TEST_OUTPUT_DIR}"
                "${source_file}" ${ARGN}
    )
    set_tests_properties("compile_${test_name}" PROPERTIES
        LABELS "compile"
        ENVIRONMENT "${JOPA_TEST_ENVIRONMENT}"
    )

    # Javap validation and runtime test (only if JVM tests are enabled)
    if(JOPA_ENABLE_JVM_TESTS)
        # Validate bytecode with javap
        add_test(
            NAME "validate_${test_name}"
            COMMAND ${Java_JAVAP_EXECUTABLE} -constants "${TEST_OUTPUT_DIR}/${main_class}.class"
        )
        set_tests_properties("validate_${test_name}" PROPERTIES
            LABELS "validate"
            DEPENDS "compile_${test_name}"
        )

        # Runtime test
        add_test(
            NAME "run_${test_name}"
            COMMAND ${TEST_JAVA_EXECUTABLE} ${TEST_JAVA_BOOTCP_FLAGS} ${JVM_TEST_FLAGS} -cp "${TEST_OUTPUT_DIR}:${RUNTIME_JAR}" "${main_class}"
        )
        set_tests_properties("run_${test_name}" PROPERTIES
            LABELS "run"
            DEPENDS "validate_${test_name}"
        )
    endif()
endfunction()

# Enum Tests
add_jopa_run_test(Color "${TEST_DIR}/enums/Color.java" "Color")
add_jopa_run_test(ColorTest "${TEST_DIR}/enums/ColorTest.java" "ColorTest" "${TEST_DIR}/enums/Color.java")
set(EnumMethodsTest_OUTPUT "${OUTPUT_DIR}/EnumMethodsTest")
file(MAKE_DIRECTORY "${EnumMethodsTest_OUTPUT}")
add_test(
    NAME "compile_EnumMethodsTest"
    COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.7 -target ${JOPA_TARGET_VERSION}
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${EnumMethodsTest_OUTPUT}"
            "${TEST_DIR}/enums/Color.java"
            "${TEST_DIR}/enums/EnumMethodsTest.java"
)
set_tests_properties("compile_EnumMethodsTest" PROPERTIES LABELS "compile")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "run_EnumMethodsTest"
        COMMAND ${TEST_JAVA_EXECUTABLE} ${TEST_JAVA_BOOTCP_FLAGS} ${JVM_TEST_FLAGS} -cp "${EnumMethodsTest_OUTPUT}:${RUNTIME_JAR}" "EnumMethodsTest"
    )
    set_tests_properties("run_EnumMethodsTest" PROPERTIES
        LABELS "run"
        DEPENDS "compile_EnumMethodsTest"
    )
endif()
add_jopa_run_test(EnumTopLevelSwitchTest "${TEST_DIR}/enums/EnumTopLevelSwitchTest.java" "EnumTopLevelSwitchTest")

# Varargs Tests
add_jopa_run_test(VarargsTest "${TEST_DIR}/varargs/VarargsTest.java" "VarargsTest")
add_jopa_run_test(VarargsImplicitTest "${TEST_DIR}/varargs/VarargsImplicitTest.java" "VarargsImplicitTest")

# Enhanced For-Loop Tests
add_jopa_run_test(ForEachTest "${TEST_DIR}/foreach/ForEachTest.java" "ForEachTest")
add_jopa_run_test(ForEachArrayTest "${TEST_DIR}/foreach/ForEachArrayTest.java" "ForEachArrayTest")
add_jopa_run_test(ForEachIterableTest "${TEST_DIR}/foreach/ForEachIterableTest.java" "ForEachIterableTest")

# Generics Tests
add_jopa_run_test(GenericBox "${TEST_DIR}/generics/GenericBox.java" "GenericBox")
add_jopa_run_test(GenericTest "${TEST_DIR}/generics/GenericTest.java" "GenericTest" "${TEST_DIR}/generics/GenericBox.java")
add_jopa_run_test(SimpleGeneric "${TEST_DIR}/generics/SimpleGeneric.java" "SimpleGeneric")
add_jopa_run_test(TwoTypeParams "${TEST_DIR}/generics/TwoTypeParams.java" "TwoTypeParams")
add_jopa_run_test(BoundedGeneric "${TEST_DIR}/generics/BoundedGeneric.java" "BoundedGeneric")
add_jopa_run_test(Wildcards "${TEST_DIR}/generics/Wildcards.java" "Wildcards")
add_jopa_run_test(GenericMethods "${TEST_DIR}/generics/GenericMethods.java" "GenericMethods")
add_jopa_run_test(CovariantReturnTest "${TEST_DIR}/generics/CovariantReturnTest.java" "CovariantReturnTest")
add_jopa_run_test(BridgeTest "${TEST_DIR}/generics/BridgeTest.java" "BridgeTest")
add_jopa_run_test(SimpleSameFileTest "${TEST_DIR}/generics/SimpleSameFileTest.java" "SimpleSameFileTest")
add_jopa_run_test(TypeSubstitutionTest "${TEST_DIR}/generics/TypeSubstitutionTest.java" "TypeSubstitutionTest")
add_jopa_run_test(FieldAccessTest "${TEST_DIR}/generics/FieldAccessTest.java" "FieldAccessTest")
add_jopa_run_test(ArrayCovarianceTest "${TEST_DIR}/generics/ArrayCovarianceTest.java" "ArrayCovarianceTest")
add_jopa_run_test(GenericVarianceTest "${TEST_DIR}/generics/GenericVarianceTest.java" "GenericVarianceTest")
add_jopa_run_test(GenericsComprehensiveTest "${TEST_DIR}/generics/GenericsComprehensiveTest.java" "GenericsComprehensiveTest")
add_jopa_run_test(RecursiveGenerics "${TEST_DIR}/generics/RecursiveGenerics.java" "RecursiveGenerics")
add_jopa_run_test(GenericReturnTypeTest "${TEST_DIR}/generics/GenericReturnTypeTest.java" "GenericReturnTypeTest")
add_jopa_run_test(ExplicitTypeArgs "${TEST_DIR}/generics/ExplicitTypeArgs.java" "ExplicitTypeArgs")
add_jopa_run_test(EnumForwardRef "${TEST_DIR}/generics/EnumForwardRef.java" "EnumForwardRef")
add_jopa_run_test(BooleanUnbox "${TEST_DIR}/generics/BooleanUnbox.java" "BooleanUnbox")
add_jopa_run_test(ArrayCloneTest "${TEST_DIR}/generics/ArrayCloneTest.java" "ArrayCloneTest")
add_jopa_run_test(EnumConstructorTest "${TEST_DIR}/generics/EnumConstructorTest.java" "EnumConstructorTest")
add_jopa_run_test(DoPrivilegedTest "${TEST_DIR}/generics/DoPrivilegedTest.java" "DoPrivilegedTest")
add_jopa_run_test(GenericMethodTest "${TEST_DIR}/generics/GenericMethodTest.java" "GenericMethodTest")
add_jopa_run_test(InterfaceTypeArgTest "${TEST_DIR}/generics/InterfaceTypeArgTest.java" "InterfaceTypeArgTest")
add_jopa_run_test(ChainedGenericsTest "${TEST_DIR}/generics/ChainedGenericsTest.java" "ChainedGenericsTest")
add_jopa_run_test(InheritedGenericTest "${TEST_DIR}/generics/InheritedGenericTest.java" "InheritedGenericTest")
add_jopa_run_test(PassthroughGenericTest "${TEST_DIR}/generics/PassthroughGenericTest.java" "PassthroughGenericTest")
add_jopa_run_test(InheritedInterfaceTest "${TEST_DIR}/generics/InheritedInterfaceTest.java" "InheritedInterfaceTest")
add_jopa_run_test(MultiInterfaceGenericsTest "${TEST_DIR}/generics/MultiInterfaceGenericsTest.java" "generics/MultiInterfaceGenericsTest")

# Autoboxing/Unboxing Tests
add_jopa_run_test(BasicBoxingTest "${TEST_DIR}/autoboxing/BasicBoxingTest.java" "BasicBoxingTest")
add_jopa_run_test(AutoboxingTest "${TEST_DIR}/autoboxing/AutoboxingTest.java" "AutoboxingTest")
add_jopa_run_test(MethodBoxingSimple "${TEST_DIR}/autoboxing/MethodBoxingSimple.java" "MethodBoxingSimple")
add_jopa_run_test(ReturnBoxingSimple "${TEST_DIR}/autoboxing/ReturnBoxingSimple.java" "ReturnBoxingSimple")
add_jopa_run_test(ArithmeticBoxingSimple "${TEST_DIR}/autoboxing/ArithmeticBoxingSimple.java" "ArithmeticBoxingSimple")
add_jopa_run_test(NumericBitwiseBoxingTest "${TEST_DIR}/autoboxing/NumericBitwiseBoxingTest.java" "NumericBitwiseBoxingTest")
add_jopa_run_test(BooleanOperatorBoxingTest "${TEST_DIR}/autoboxing/BooleanOperatorBoxingTest.java" "BooleanOperatorBoxingTest")

# Varargs Tests
add_jopa_run_test(VarargsMethodTest "${TEST_DIR}/varargs/VarargsMethodTest.java" "VarargsMethodTest")

# Static Import Tests
set(MathUtils_OUTPUT "${OUTPUT_DIR}/MathUtils")
file(MAKE_DIRECTORY "${MathUtils_OUTPUT}")
add_test(
    NAME "compile_MathUtils"
    COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.7 -target ${JOPA_TARGET_VERSION}
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${MathUtils_OUTPUT}"
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

# Debug info tests (compiled with -g flag for parameter names)
set(SimpleJava6Test_OUTPUT "${OUTPUT_DIR}/SimpleJava6Test")
file(MAKE_DIRECTORY "${SimpleJava6Test_OUTPUT}")
add_test(
    NAME "compile_SimpleJava6Test"
    COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.7 -target ${JOPA_TARGET_VERSION} -g
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${SimpleJava6Test_OUTPUT}"
            "${TEST_DIR}/java6/SimpleJava6Test.java"
)
set_tests_properties("compile_SimpleJava6Test" PROPERTIES LABELS "compile")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "run_SimpleJava6Test"
        COMMAND ${TEST_JAVA_EXECUTABLE} ${TEST_JAVA_BOOTCP_FLAGS} ${JVM_TEST_FLAGS} -cp "${SimpleJava6Test_OUTPUT}:${RUNTIME_JAR}" "SimpleJava6Test"
    )
    set_tests_properties("run_SimpleJava6Test" PROPERTIES
        LABELS "run"
        DEPENDS "compile_SimpleJava6Test"
    )
endif()

# Reflection Tests (compile with our runtime, run with system JVM for real reflection)
set(ReflectionBasicTest_OUTPUT "${OUTPUT_DIR}/ReflectionBasicTest")
file(MAKE_DIRECTORY "${ReflectionBasicTest_OUTPUT}")
add_test(
    NAME "compile_ReflectionBasicTest"
    COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.7 -target ${JOPA_TARGET_VERSION} -g
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${ReflectionBasicTest_OUTPUT}"
            "${TEST_DIR}/java6/ReflectionBasicTest.java"
)
set_tests_properties("compile_ReflectionBasicTest" PROPERTIES LABELS "compile")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "run_ReflectionBasicTest"
        COMMAND ${TEST_JAVA_EXECUTABLE} ${TEST_JAVA_BOOTCP_FLAGS} ${JVM_TEST_FLAGS} -cp "${ReflectionBasicTest_OUTPUT}" "ReflectionBasicTest"
    )
    set_tests_properties("run_ReflectionBasicTest" PROPERTIES
        LABELS "run"
        DEPENDS "compile_ReflectionBasicTest"
    )
endif()

# Comprehensive Reflection Test
add_jopa_run_test(ReflectionComprehensiveTest "${TEST_DIR}/reflection/ReflectionComprehensiveTest.java" "ReflectionComprehensiveTest")

# Multi-File Compilation Tests
set(MultiFileTest_OUTPUT "${OUTPUT_DIR}/MultiFileTest")
file(MAKE_DIRECTORY "${MultiFileTest_OUTPUT}")
add_test(
    NAME "compile_MultiFileTest"
    COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.7 -target ${JOPA_TARGET_VERSION}
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${MultiFileTest_OUTPUT}"
            "${TEST_DIR}/multifile/MultiFileTest.java"
            "${TEST_DIR}/multifile/Service.java"
            "${TEST_DIR}/multifile/ServiceImpl.java"
)
set_tests_properties("compile_MultiFileTest" PROPERTIES LABELS "compile;multifile")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "validate_MultiFileTest"
        COMMAND ${Java_JAVAP_EXECUTABLE} -constants "${MultiFileTest_OUTPUT}/MultiFileTest.class"
    )
    set_tests_properties("validate_MultiFileTest" PROPERTIES
        LABELS "validate;multifile"
        DEPENDS "compile_MultiFileTest"
    )
    add_test(
        NAME "run_MultiFileTest"
        COMMAND ${TEST_JAVA_EXECUTABLE} ${TEST_JAVA_BOOTCP_FLAGS} ${JVM_TEST_FLAGS} -cp "${MultiFileTest_OUTPUT}:${RUNTIME_JAR}" "MultiFileTest"
    )
    set_tests_properties("run_MultiFileTest" PROPERTIES
        LABELS "run;multifile"
        DEPENDS "validate_MultiFileTest"
    )
endif()

# Anonymous Class Tests
add_jopa_run_test(AnonymousClassTest "${TEST_DIR}/anonymous/AnonymousClassTest.java" "AnonymousClassTest")
add_jopa_run_test(AnonymousGenericTest "${TEST_DIR}/anonymous/AnonymousGenericTest.java" "AnonymousGenericTest")
add_jopa_run_test(TypeTokenTest "${TEST_DIR}/anonymous/TypeTokenTest.java" "TypeTokenTest")
add_jopa_run_test(CapturedIteratorTest "${TEST_DIR}/anonymous/CapturedIteratorTest.java" "CapturedIteratorTest")

# Chained Generic Return Type Test
add_jopa_run_test(ChainedGenericReturnType "${TEST_DIR}/ChainedGenericReturnType.java" "ChainedGenericReturnType")

# Enum Constants Type Parameter Array Test
add_jopa_run_test(EnumConstantsTest "${TEST_DIR}/EnumConstantsTest.java" "EnumConstantsTest")

# StackMapTable Verification Tests
# These tests exercise control flow patterns that require proper StackMapTable
# generation for Java 7+ bytecode (class version 51.0+).
add_jopa_run_test(StackMapTableTest "${TEST_DIR}/stackmap/StackMapTableTest.java" "StackMapTableTest")

# Java 7 Tests (diamond, multi-catch, try-with-resources, string switch)

# Diamond Operator Test
set(DiamondTest_OUTPUT "${OUTPUT_DIR}/DiamondTest")
file(MAKE_DIRECTORY "${DiamondTest_OUTPUT}")
add_test(
    NAME "compile_DiamondTest"
    COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.7 -target ${JOPA_TARGET_VERSION}
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${DiamondTest_OUTPUT}"
            "${TEST_DIR}/java7/DiamondTest.java"
)
set_tests_properties("compile_DiamondTest" PROPERTIES LABELS "compile;java7")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "validate_DiamondTest"
        COMMAND ${Java_JAVAP_EXECUTABLE} -constants "${DiamondTest_OUTPUT}/DiamondTest.class"
    )
    set_tests_properties("validate_DiamondTest" PROPERTIES
        LABELS "validate;java7"
        DEPENDS "compile_DiamondTest"
    )
    add_test(
        NAME "run_DiamondTest"
        COMMAND ${TEST_JAVA_EXECUTABLE} ${TEST_JAVA_BOOTCP_FLAGS} ${JVM_TEST_FLAGS} -cp "${DiamondTest_OUTPUT}:${RUNTIME_JAR}" "DiamondTest"
    )
    set_tests_properties("run_DiamondTest" PROPERTIES
        LABELS "run;java7"
        DEPENDS "validate_DiamondTest"
    )
endif()

# Multi-Catch Test
set(MultiCatchTest_OUTPUT "${OUTPUT_DIR}/MultiCatchTest")
file(MAKE_DIRECTORY "${MultiCatchTest_OUTPUT}")
add_test(
    NAME "compile_MultiCatchTest"
    COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.7 -target ${JOPA_TARGET_VERSION}
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${MultiCatchTest_OUTPUT}"
            "${TEST_DIR}/java7/MultiCatchTest.java"
)
set_tests_properties("compile_MultiCatchTest" PROPERTIES LABELS "compile;java7")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "validate_MultiCatchTest"
        COMMAND ${Java_JAVAP_EXECUTABLE} -constants "${MultiCatchTest_OUTPUT}/MultiCatchTest.class"
    )
    set_tests_properties("validate_MultiCatchTest" PROPERTIES
        LABELS "validate;java7"
        DEPENDS "compile_MultiCatchTest"
    )
    add_test(
        NAME "run_MultiCatchTest"
        COMMAND ${TEST_JAVA_EXECUTABLE} ${TEST_JAVA_BOOTCP_FLAGS} ${JVM_TEST_FLAGS} -cp "${MultiCatchTest_OUTPUT}:${RUNTIME_JAR}" "MultiCatchTest"
    )
    set_tests_properties("run_MultiCatchTest" PROPERTIES
        LABELS "run;java7"
        DEPENDS "validate_MultiCatchTest"
    )
endif()

# Try-with-resources Test
set(TryWithResourcesTest_OUTPUT "${OUTPUT_DIR}/TryWithResourcesTest")
file(MAKE_DIRECTORY "${TryWithResourcesTest_OUTPUT}")
add_test(
    NAME "compile_TryWithResourcesTest"
    COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.7 -target ${JOPA_TARGET_VERSION}
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${TryWithResourcesTest_OUTPUT}"
            "${TEST_DIR}/java7/TryWithResourcesTest.java"
)
set_tests_properties("compile_TryWithResourcesTest" PROPERTIES LABELS "compile;java7")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "validate_TryWithResourcesTest"
        COMMAND ${Java_JAVAP_EXECUTABLE} -constants "${TryWithResourcesTest_OUTPUT}/TryWithResourcesTest.class"
    )
    set_tests_properties("validate_TryWithResourcesTest" PROPERTIES
        LABELS "validate;java7"
        DEPENDS "compile_TryWithResourcesTest"
    )
    
    add_test(
        NAME "run_TryWithResourcesTest"
        COMMAND ${TEST_JAVA_EXECUTABLE} ${TEST_JAVA_BOOTCP_FLAGS} ${JVM_TEST_FLAGS} -cp "${TryWithResourcesTest_OUTPUT}:${RUNTIME_JAR}" "TryWithResourcesTest"
    )
    set_tests_properties("run_TryWithResourcesTest" PROPERTIES
        LABELS "run;java7"
        DEPENDS "validate_TryWithResourcesTest"
    )
endif()

# Java 7 String Switch Test
set(StringSwitchTest_OUTPUT "${OUTPUT_DIR}/StringSwitchTest")
file(MAKE_DIRECTORY "${StringSwitchTest_OUTPUT}")
add_test(
    NAME "compile_StringSwitchTest"
    COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.7 -target ${JOPA_TARGET_VERSION}
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${StringSwitchTest_OUTPUT}"
            "${TEST_DIR}/java7/StringSwitchTest.java"
)
set_tests_properties("compile_StringSwitchTest" PROPERTIES LABELS "compile;java7")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "validate_StringSwitchTest"
        COMMAND ${Java_JAVAP_EXECUTABLE} -constants "${StringSwitchTest_OUTPUT}/StringSwitchTest.class"
    )
    set_tests_properties("validate_StringSwitchTest" PROPERTIES
        LABELS "validate;java7"
        DEPENDS "compile_StringSwitchTest"
    )
    add_test(
        NAME "run_StringSwitchTest"
        COMMAND ${TEST_JAVA_EXECUTABLE} ${TEST_JAVA_BOOTCP_FLAGS} ${JVM_TEST_FLAGS} -cp "${StringSwitchTest_OUTPUT}:${RUNTIME_JAR}" "StringSwitchTest"
    )
    set_tests_properties("run_StringSwitchTest" PROPERTIES
        LABELS "run;java7"
        DEPENDS "validate_StringSwitchTest"
    )
endif()

# Enum Switch Test
add_jopa_run_test(EnumSwitchTest "${TEST_DIR}/java7/EnumSwitchTest.java" "EnumSwitchTest")

# Java 8 Tests
# Tests default methods (partial Java 8 support).
# Note: JOPA supports Java 7 bytecode (v51.0) with StackMapTable generation.
# Tests pass with -target 1.5/1.6 (no StackMapTable required) and with
# -target 1.7 (StackMapTable generation, though some complex control flow patterns
# may have verification issues requiring -noverify).
set(DefaultMethodTest_OUTPUT "${OUTPUT_DIR}/DefaultMethodTest")
file(MAKE_DIRECTORY "${DefaultMethodTest_OUTPUT}")
add_test(
    NAME "compile_DefaultMethodTest"
    COMMAND $<TARGET_FILE:jopa> ${JOPA_EXTRA_FLAGS} -source 1.8 -target 1.8
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_JAR}"
            -d "${DefaultMethodTest_OUTPUT}"
            "${TEST_DIR}/java8/DefaultMethodTest.java"
)
set_tests_properties("compile_DefaultMethodTest" PROPERTIES LABELS "compile;java8")
if(JOPA_ENABLE_JVM_TESTS)
    add_test(
        NAME "validate_DefaultMethodTest"
        COMMAND ${Java_JAVAP_EXECUTABLE} -constants "${DefaultMethodTest_OUTPUT}/DefaultMethodTest.class"
    )
    set_tests_properties("validate_DefaultMethodTest" PROPERTIES
        LABELS "validate;java8"
        DEPENDS "compile_DefaultMethodTest"
    )
    add_test(
        NAME "run_DefaultMethodTest"
        COMMAND ${TEST_JAVA_EXECUTABLE} ${TEST_JAVA_BOOTCP_FLAGS} ${JVM_TEST_FLAGS} -cp "${DefaultMethodTest_OUTPUT}:${RUNTIME_JAR}" "DefaultMethodTest"
    )
    set_tests_properties("run_DefaultMethodTest" PROPERTIES
        LABELS "run;java8"
        DEPENDS "validate_DefaultMethodTest"
    )
endif()

# ============================================================================
# JDK Language Tools Parser Tests
# ============================================================================
# Whitelists are generated by: scripts/prepare_jdk_parser_tests.py

if(JOPA_ENABLE_JDK_PARSER_TESTS)
    set(PARSER_TEST_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/parser_tests")
    file(MAKE_DIRECTORY "${PARSER_TEST_OUTPUT_DIR}")

    # Function to add parser tests for a JDK version from whitelist
    function(add_jdk_parser_tests jdk_version jdk_assets_dir whitelist_file)
        if(NOT EXISTS "${jdk_assets_dir}")
            message(WARNING "JDK${jdk_version} assets not found at ${jdk_assets_dir}")
            return()
        endif()

        if(NOT EXISTS "${whitelist_file}")
            message(WARNING "JDK${jdk_version} whitelist not found at ${whitelist_file}")
            message(WARNING "Run: python scripts/prepare_jdk_parser_tests.py --jdk${jdk_version}")
            return()
        endif()

        # Count total .java files for reporting
        file(GLOB_RECURSE ALL_JAVA_FILES "${jdk_assets_dir}/*.java")
        list(LENGTH ALL_JAVA_FILES total_count)

        # Read whitelist file
        file(STRINGS "${whitelist_file}" WHITELIST_ENTRIES)

        set(test_count 0)
        foreach(rel_path ${WHITELIST_ENTRIES})
            # Skip empty lines
            if("${rel_path}" STREQUAL "")
                continue()
            endif()

            set(java_file "${jdk_assets_dir}/${rel_path}")

            # Create test name from relative path
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
                TIMEOUT 10
            )

            math(EXPR test_count "${test_count} + 1")
        endforeach()

        message(STATUS "JDK${jdk_version} parser tests: ${test_count} whitelisted / ${total_count} total")
    endfunction()

    # Add JDK7 parser tests
    if(JOPA_PARSER_TESTS_JDK7)
        add_jdk_parser_tests(7
            "${CMAKE_SOURCE_DIR}/assets/jdk7u-langtools/test"
            "${CMAKE_CURRENT_SOURCE_DIR}/jdk7_parser_whitelist.txt")
    endif()

    # Add JDK8 parser tests
    if(JOPA_PARSER_TESTS_JDK8)
        add_jdk_parser_tests(8
            "${CMAKE_SOURCE_DIR}/assets/jdk8u_langtools/test"
            "${CMAKE_CURRENT_SOURCE_DIR}/jdk8_parser_whitelist.txt")
    endif()
endif()