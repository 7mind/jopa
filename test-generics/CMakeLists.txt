# Java 5 Feature Tests for Jikes Compiler
# Tests both compilation and runtime execution with real JVM

enable_testing()

# Find Java for runtime tests
find_package(Java COMPONENTS Runtime REQUIRED)

set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(RUNTIME_DIR "${TEST_DIR}/runtime")
set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}")

# Helper function to compile Java runtime classes
function(compile_runtime_classes)
    set(RUNTIME_SOURCES
        "${RUNTIME_DIR}/java/util/Iterator.java"
        "${RUNTIME_DIR}/java/lang/Iterable.java"
        "${RUNTIME_DIR}/java/util/ArrayList.java"
    )

    add_custom_command(
        OUTPUT "${RUNTIME_DIR}/java/util/Iterator.class"
               "${RUNTIME_DIR}/java/lang/Iterable.class"
               "${RUNTIME_DIR}/java/util/ArrayList.class"
        COMMAND $<TARGET_FILE:jikes> -source 1.5 -sourcepath "${RUNTIME_DIR}" -d "${RUNTIME_DIR}" ${RUNTIME_SOURCES}
        DEPENDS jikes ${RUNTIME_SOURCES}
        COMMENT "Compiling Java runtime classes"
    )

    add_custom_target(runtime_classes ALL
        DEPENDS "${RUNTIME_DIR}/java/util/Iterator.class"
                "${RUNTIME_DIR}/java/lang/Iterable.class"
                "${RUNTIME_DIR}/java/util/ArrayList.class"
    )
endfunction()

# Helper function to add a compile-only test
function(add_jikes_compile_test test_name source_file)
    add_test(
        NAME "compile_${test_name}"
        COMMAND $<TARGET_FILE:jikes> -source 1.5
                -sourcepath "${RUNTIME_DIR}:${TEST_DIR}"
                -classpath "${RUNTIME_DIR}"
                -d "${OUTPUT_DIR}"
                "${TEST_DIR}/${source_file}"
    )
    set_tests_properties("compile_${test_name}" PROPERTIES
        LABELS "compile"
    )
endfunction()

# Helper function to add a compile+run test
function(add_jikes_run_test test_name source_file main_class)
    set(class_file "${OUTPUT_DIR}/${main_class}.class")

    # Compilation test
    add_test(
        NAME "compile_${test_name}"
        COMMAND $<TARGET_FILE:jikes> -source 1.5
                -sourcepath "${RUNTIME_DIR}:${TEST_DIR}"
                -classpath "${RUNTIME_DIR}"
                -d "${OUTPUT_DIR}"
                "${TEST_DIR}/${source_file}"
    )
    set_tests_properties("compile_${test_name}" PROPERTIES
        LABELS "compile"
    )

    # Runtime test
    add_test(
        NAME "run_${test_name}"
        COMMAND ${Java_JAVA_EXECUTABLE} -cp "${OUTPUT_DIR}:${RUNTIME_DIR}" "${main_class}"
    )
    set_tests_properties("run_${test_name}" PROPERTIES
        LABELS "run"
        DEPENDS "compile_${test_name}"
    )
endfunction()

# Compile runtime classes first
compile_runtime_classes()

# Enum Tests
add_jikes_compile_test(Color "Color.java")
add_jikes_run_test(ColorTest "ColorTest.java" "ColorTest")
add_test(
    NAME "compile_EnumMethodsTest"
    COMMAND $<TARGET_FILE:jikes> -source 1.5
            -sourcepath "${RUNTIME_DIR}:${TEST_DIR}"
            -classpath "${RUNTIME_DIR}"
            -d "${OUTPUT_DIR}"
            "${TEST_DIR}/Color.java"
            "${TEST_DIR}/EnumMethodsTest.java"
)
set_tests_properties("compile_EnumMethodsTest" PROPERTIES LABELS "compile")
add_test(
    NAME "run_EnumMethodsTest"
    COMMAND ${Java_JAVA_EXECUTABLE} -cp "${OUTPUT_DIR}:${RUNTIME_DIR}" "EnumMethodsTest"
)
set_tests_properties("run_EnumMethodsTest" PROPERTIES
    LABELS "run"
    DEPENDS "compile_EnumMethodsTest"
)

# Varargs Tests
add_jikes_run_test(VarargsTest "VarargsTest.java" "VarargsTest")
add_jikes_compile_test(VarargsImplicitTest "VarargsImplicitTest.java")

# Enhanced For-Loop Tests
add_jikes_run_test(ForEachTest "ForEachTest.java" "ForEachTest")
add_jikes_compile_test(ForEachArrayTest "ForEachArrayTest.java")
add_jikes_run_test(ForEachIterableTest "ForEachIterableTest.java" "ForEachIterableTest")

# Generics Tests
add_jikes_compile_test(GenericBox "GenericBox.java")
add_jikes_run_test(GenericTest "GenericTest.java" "GenericTest")
add_jikes_compile_test(SimpleGeneric "SimpleGeneric.java")
add_jikes_compile_test(TwoTypeParams "TwoTypeParams.java")
add_jikes_compile_test(BoundedGeneric "BoundedGeneric.java")

# Autoboxing/Unboxing Tests
add_jikes_run_test(BasicBoxingTest "BasicBoxingTest.java" "BasicBoxingTest")
add_jikes_run_test(AutoboxingTest "AutoboxingTest.java" "AutoboxingTest")
add_jikes_run_test(MethodBoxingSimple "MethodBoxingSimple.java" "MethodBoxingSimple")
add_jikes_run_test(ReturnBoxingSimple "ReturnBoxingSimple.java" "ReturnBoxingSimple")
add_jikes_run_test(ArithmeticBoxingSimple "ArithmeticBoxingSimple.java" "ArithmeticBoxingSimple")

# Static Import Tests
# First compile MathUtils
add_test(
    NAME "compile_MathUtils"
    COMMAND $<TARGET_FILE:jikes> -source 1.5
            -sourcepath "${RUNTIME_DIR}:${TEST_DIR}"
            -classpath "${RUNTIME_DIR}"
            -d "${OUTPUT_DIR}"
            "${TEST_DIR}/util/MathUtils.java"
)
set_tests_properties("compile_MathUtils" PROPERTIES LABELS "compile")

add_jikes_run_test(StaticFieldOnlyTest "StaticFieldOnlyTest.java" "StaticFieldOnlyTest")
add_jikes_run_test(StaticMethodImportTest "StaticMethodImportTest.java" "StaticMethodImportTest")
add_jikes_run_test(StaticWildcardImportTest "StaticWildcardImportTest.java" "StaticWildcardImportTest")
