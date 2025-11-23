# Java 5 Feature Tests for Jikes Compiler
# Tests both compilation and runtime execution with real JVM

enable_testing()

# Option to enable/disable JVM validation tests
option(JIKES_ENABLE_JVM_TESTS "Enable runtime tests using real JVM (requires Java)" ON)

# Find Java for runtime tests (only if JVM tests are enabled)
if(JIKES_ENABLE_JVM_TESTS)
    find_package(Java COMPONENTS Runtime REQUIRED)
endif()

set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUNTIME_SRC_DIR "${CMAKE_SOURCE_DIR}/runtime")
set(RUNTIME_DIR "${CMAKE_BINARY_DIR}/runtime")
set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Helper function to add a compile-only test
function(add_jikes_compile_test test_name source_file)
    add_test(
        NAME "compile_${test_name}"
        COMMAND $<TARGET_FILE:jikes> -source 1.5
                -sourcepath "${TEST_DIR}"
                -classpath "${RUNTIME_DIR}"
                -d "${OUTPUT_DIR}"
                "${source_file}"
    )
    set_tests_properties("compile_${test_name}" PROPERTIES
        LABELS "compile"
    )
endfunction()

# Helper function to add a compile+run test
function(add_jikes_run_test test_name source_file main_class)
    # Compilation test
    add_test(
        NAME "compile_${test_name}"
        COMMAND $<TARGET_FILE:jikes> -source 1.5
                -sourcepath "${TEST_DIR}"
                -classpath "${RUNTIME_DIR}"
                -d "${OUTPUT_DIR}"
                "${source_file}"
    )
    set_tests_properties("compile_${test_name}" PROPERTIES
        LABELS "compile"
    )

    # Runtime test (only if JVM tests are enabled)
    if(JIKES_ENABLE_JVM_TESTS)
        add_test(
            NAME "run_${test_name}"
            COMMAND ${Java_JAVA_EXECUTABLE} -cp "${OUTPUT_DIR}:${RUNTIME_DIR}" "${main_class}"
        )
        set_tests_properties("run_${test_name}" PROPERTIES
            LABELS "run"
            DEPENDS "compile_${test_name}"
        )
    endif()
endfunction()

# Enum Tests
add_jikes_compile_test(Color "${TEST_DIR}/enums/Color.java")
add_jikes_run_test(ColorTest "${TEST_DIR}/enums/ColorTest.java" "ColorTest")
add_test(
    NAME "compile_EnumMethodsTest"
    COMMAND $<TARGET_FILE:jikes> -source 1.5
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_DIR}"
            -d "${OUTPUT_DIR}"
            "${TEST_DIR}/enums/Color.java"
            "${TEST_DIR}/enums/EnumMethodsTest.java"
)
set_tests_properties("compile_EnumMethodsTest" PROPERTIES LABELS "compile")
if(JIKES_ENABLE_JVM_TESTS)
    add_test(
        NAME "run_EnumMethodsTest"
        COMMAND ${Java_JAVA_EXECUTABLE} -cp "${OUTPUT_DIR}:${RUNTIME_DIR}" "EnumMethodsTest"
    )
    set_tests_properties("run_EnumMethodsTest" PROPERTIES
        LABELS "run"
        DEPENDS "compile_EnumMethodsTest"
    )
endif()

# Varargs Tests
add_jikes_run_test(VarargsTest "${TEST_DIR}/varargs/VarargsTest.java" "VarargsTest")
add_jikes_compile_test(VarargsImplicitTest "${TEST_DIR}/varargs/VarargsImplicitTest.java")

# Enhanced For-Loop Tests
add_jikes_run_test(ForEachTest "${TEST_DIR}/foreach/ForEachTest.java" "ForEachTest")
add_jikes_compile_test(ForEachArrayTest "${TEST_DIR}/foreach/ForEachArrayTest.java")
add_jikes_run_test(ForEachIterableTest "${TEST_DIR}/foreach/ForEachIterableTest.java" "ForEachIterableTest")

# Generics Tests
add_jikes_compile_test(GenericBox "${TEST_DIR}/generics/GenericBox.java")
add_jikes_run_test(GenericTest "${TEST_DIR}/generics/GenericTest.java" "GenericTest")
add_jikes_compile_test(SimpleGeneric "${TEST_DIR}/generics/SimpleGeneric.java")
add_jikes_compile_test(TwoTypeParams "${TEST_DIR}/generics/TwoTypeParams.java")
add_jikes_compile_test(BoundedGeneric "${TEST_DIR}/generics/BoundedGeneric.java")
add_jikes_compile_test(Wildcards "${TEST_DIR}/generics/Wildcards.java")
add_jikes_compile_test(GenericMethods "${TEST_DIR}/generics/GenericMethods.java")
add_jikes_compile_test(CovariantReturnTest "${TEST_DIR}/generics/CovariantReturnTest.java")
add_jikes_compile_test(BridgeTest "${TEST_DIR}/generics/BridgeTest.java")
add_jikes_compile_test(SimpleSameFileTest "${TEST_DIR}/generics/SimpleSameFileTest.java")
add_jikes_compile_test(TypeSubstitutionTest "${TEST_DIR}/generics/TypeSubstitutionTest.java")
add_jikes_compile_test(FieldAccessTest "${TEST_DIR}/generics/FieldAccessTest.java")
add_jikes_run_test(ArrayCovarianceTest "${TEST_DIR}/generics/ArrayCovarianceTest.java" "ArrayCovarianceTest")
add_jikes_run_test(GenericVarianceTest "${TEST_DIR}/generics/GenericVarianceTest.java" "GenericVarianceTest")

# Autoboxing/Unboxing Tests
add_jikes_run_test(BasicBoxingTest "${TEST_DIR}/autoboxing/BasicBoxingTest.java" "BasicBoxingTest")
add_jikes_run_test(AutoboxingTest "${TEST_DIR}/autoboxing/AutoboxingTest.java" "AutoboxingTest")
add_jikes_run_test(MethodBoxingSimple "${TEST_DIR}/autoboxing/MethodBoxingSimple.java" "MethodBoxingSimple")
add_jikes_run_test(ReturnBoxingSimple "${TEST_DIR}/autoboxing/ReturnBoxingSimple.java" "ReturnBoxingSimple")
add_jikes_run_test(ArithmeticBoxingSimple "${TEST_DIR}/autoboxing/ArithmeticBoxingSimple.java" "ArithmeticBoxingSimple")

# Static Import Tests
add_test(
    NAME "compile_MathUtils"
    COMMAND $<TARGET_FILE:jikes> -source 1.5
            -sourcepath "${TEST_DIR}"
            -classpath "${RUNTIME_DIR}"
            -d "${OUTPUT_DIR}"
            "${TEST_DIR}/util/MathUtils.java"
)
set_tests_properties("compile_MathUtils" PROPERTIES LABELS "compile")

add_jikes_run_test(StaticFieldOnlyTest "${TEST_DIR}/static-imports/StaticFieldOnlyTest.java" "StaticFieldOnlyTest")
add_jikes_run_test(StaticMethodImportTest "${TEST_DIR}/static-imports/StaticMethodImportTest.java" "StaticMethodImportTest")
add_jikes_run_test(StaticWildcardImportTest "${TEST_DIR}/static-imports/StaticWildcardImportTest.java" "StaticWildcardImportTest")

# Annotation Tests
add_jikes_run_test(AnnotationTest "${TEST_DIR}/annotations/AnnotationTest.java" "AnnotationTest")
add_jikes_run_test(MarkerAnnotationTest "${TEST_DIR}/annotations/MarkerAnnotationTest.java" "MarkerAnnotationTest")
add_jikes_run_test(CustomAnnotationTest "${TEST_DIR}/annotations/CustomAnnotationTest.java" "CustomAnnotationTest")
add_jikes_run_test(SuppressWarningsTest "${TEST_DIR}/annotations/SuppressWarningsTest.java" "SuppressWarningsTest")
