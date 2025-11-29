# Vendor dependencies: JamVM + GNU Classpath
#
# Full no-blob bootstrap chain - ALL Java bytecode compiled by JOPA:
# 1. Build JOPA (C++ compiler)
# 2. Build GNU Classpath with JOPA -> glibj.zip (compiled by JOPA)
# 3. Build JamVM (C code + classes.zip compiled by JOPA)
#
# Result: Complete Java toolchain with no prebuilt Java bytecode blobs
# Note: No system JDK required - we use 'zip' instead of 'jar' for packaging

include(ExternalProject)

find_program(MAKE_COMMAND make REQUIRED)
find_program(TAR_COMMAND tar REQUIRED)
find_program(ZIP_COMMAND zip REQUIRED)

set(VENDOR_PREFIX "${CMAKE_BINARY_DIR}/vendor-install")
file(MAKE_DIRECTORY "${VENDOR_PREFIX}")

include(ProcessorCount)
ProcessorCount(NPROC)
if(NPROC EQUAL 0)
    set(NPROC 1)
endif()

# ============================================================================
# Extract archives to build directory (out-of-tree)
# ============================================================================
set(CLASSPATH_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/classpath-0.99.tar.gz")
set(JAMVM_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/jamvm-2.0.0.tar.gz")
set(CLASSPATH_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/classpath-0.99")
set(JAMVM_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/jamvm-2.0.0")

# Extract GNU Classpath if not present
if(NOT EXISTS "${CLASSPATH_SOURCE_DIR}/configure")
    message(STATUS "Extracting GNU Classpath to ${CLASSPATH_SOURCE_DIR}")
    execute_process(
        COMMAND ${TAR_COMMAND} -xzf "${CLASSPATH_ARCHIVE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        RESULT_VARIABLE TAR_RESULT
    )
    if(NOT TAR_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to extract GNU Classpath archive")
    endif()
endif()

# Extract JamVM if not present
if(NOT EXISTS "${JAMVM_SOURCE_DIR}/configure")
    message(STATUS "Extracting JamVM to ${JAMVM_SOURCE_DIR}")
    execute_process(
        COMMAND ${TAR_COMMAND} -xzf "${JAMVM_ARCHIVE}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        RESULT_VARIABLE TAR_RESULT
    )
    if(NOT TAR_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to extract JamVM archive")
    endif()
endif()

# ============================================================================
# JOPA compiler and runtime paths (built as part of main project)
# ============================================================================
set(JOPA_EXECUTABLE "${CMAKE_BINARY_DIR}/src/jopa")
set(JOPA_RUNTIME_JAR "${CMAKE_BINARY_DIR}/runtime/jopa-stub-rt.jar")

# Clean Java environment to prevent any system Java from leaking in
set(CLEAN_JAVA_ENV_VARS
    "--unset=JAVA_HOME"
    "--unset=JDK_HOME"
    "--unset=CLASSPATH"
    "--unset=JAVA"
    "--unset=JAVAC"
    "--unset=JAR"
)

# ============================================================================
# GNU Classpath Build - compiled with JOPA
# ============================================================================
set(CLASSPATH_BUILD_DIR "${CMAKE_BINARY_DIR}/classpath-build")
set(CLASSPATH_INSTALL_DIR "${VENDOR_PREFIX}/classpath")

ExternalProject_Add(gnu_classpath
    SOURCE_DIR "${CLASSPATH_SOURCE_DIR}"
    BINARY_DIR "${CLASSPATH_BUILD_DIR}"
    INSTALL_DIR "${CLASSPATH_INSTALL_DIR}"

    # Use JOPA as the Java compiler with our stub runtime.jar as bootclasspath
    # This provides java.lang.Object, String, etc. needed for compilation
    # -nowarn:unchecked suppresses raw type warnings (GNU Classpath uses pre-generics style)
    # Using ASAN build with UBSAN errors and leak detection suppressed
    # --with-jar=no forces use of ZIP instead of jar tool (no JDK dependency)
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        "UBSAN_OPTIONS=halt_on_error=0"
        "ASAN_OPTIONS=detect_leaks=0"
        "JAVAC=${JOPA_EXECUTABLE} --nowarn:unchecked -bootclasspath ${JOPA_RUNTIME_JAR}"
        "ZIP=${ZIP_COMMAND}"
        "ECJ="
        "${CLASSPATH_SOURCE_DIR}/configure"
            --prefix=<INSTALL_DIR>
            --with-jar=no
            --disable-gtk-peer
            --disable-gconf-peer
            --disable-plugin
            --disable-examples
            --disable-Werror
            --disable-gjdoc

    BUILD_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        "UBSAN_OPTIONS=halt_on_error=0"
        "ASAN_OPTIONS=detect_leaks=0"
        "JAVAC=${JOPA_EXECUTABLE} --nowarn:unchecked -bootclasspath ${JOPA_RUNTIME_JAR}"
        "ZIP=${ZIP_COMMAND}"
        ${MAKE_COMMAND} -j${NPROC}

    INSTALL_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        ${MAKE_COMMAND} install

    BUILD_BYPRODUCTS "${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip"
)

# GNU Classpath depends on JOPA and runtime.jar being built first
ExternalProject_Add_StepDependencies(gnu_classpath configure jopa jopa-stub-rt)

set(CLASSPATH_GLIBJ_ZIP "${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip" PARENT_SCOPE)

# ============================================================================
# JamVM Build - classes.zip compiled with JOPA
# ============================================================================
set(JAMVM_BUILD_DIR "${CMAKE_BINARY_DIR}/jamvm-build")
set(JAMVM_INSTALL_DIR "${VENDOR_PREFIX}/jamvm")

# Delete prebuilt classes.zip to force rebuild from source using JOPA
set(JAMVM_PREBUILT_CLASSES "${JAMVM_SOURCE_DIR}/src/classlib/gnuclasspath/lib/classes.zip")

ExternalProject_Add(jamvm
    SOURCE_DIR "${JAMVM_SOURCE_DIR}"
    BINARY_DIR "${JAMVM_BUILD_DIR}"
    INSTALL_DIR "${JAMVM_INSTALL_DIR}"

    DEPENDS gnu_classpath

    # Delete prebuilt classes.zip, then configure with JOPA as compiler
    # Use GNU Classpath's glibj.zip as bootclasspath for JamVM classes
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E rm -f "${JAMVM_PREBUILT_CLASSES}"
    COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        "UBSAN_OPTIONS=halt_on_error=0"
        "ASAN_OPTIONS=detect_leaks=0"
        "JAVAC=${JOPA_EXECUTABLE} --nowarn:unchecked -bootclasspath ${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip"
        "${JAMVM_SOURCE_DIR}/configure"
            --prefix=<INSTALL_DIR>
            --with-java-runtime-library=gnuclasspath
            --with-classpath-install-dir=${CLASSPATH_INSTALL_DIR}
            --enable-int-inlining

    BUILD_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        "UBSAN_OPTIONS=halt_on_error=0"
        "ASAN_OPTIONS=detect_leaks=0"
        "JAVAC=${JOPA_EXECUTABLE} --nowarn:unchecked -bootclasspath ${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip"
        ${MAKE_COMMAND} -j${NPROC}

    INSTALL_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        ${MAKE_COMMAND} install

    BUILD_BYPRODUCTS "${JAMVM_INSTALL_DIR}/bin/jamvm"
)

set(JAMVM_EXECUTABLE "${JAMVM_INSTALL_DIR}/bin/jamvm" PARENT_SCOPE)
set(CLASSPATH_INSTALL_PREFIX "${CLASSPATH_INSTALL_DIR}" PARENT_SCOPE)

# jamvm_with_gnucp is the main target for building the complete JVM stack
add_custom_target(jamvm_with_gnucp DEPENDS jamvm)

# JamVM implicitly depends on jopa through gnu_classpath dependency chain

message(STATUS "Bootstrap: ALL Java bytecode will be compiled by JOPA")
message(STATUS "JamVM sources: ${JAMVM_SOURCE_DIR}")
message(STATUS "GNU Classpath sources: ${CLASSPATH_SOURCE_DIR}")
message(STATUS "Install prefix: ${VENDOR_PREFIX}")
