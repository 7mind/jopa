# Vendor dependencies: JamVM + GNU Classpath
#
# Bootstrap chain:
# 1. Build GNU Classpath (with system javac) -> glibj.zip
# 2. Build JamVM (C code) -> jamvm binary
# 3. Later: rebuild glibj.zip with JOPA for full bootstrap

include(ExternalProject)

find_program(MAKE_COMMAND make REQUIRED)
find_program(TAR_COMMAND tar REQUIRED)
find_package(Java COMPONENTS Development REQUIRED)

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
# GNU Classpath Build
# ============================================================================
set(CLASSPATH_BUILD_DIR "${CMAKE_BINARY_DIR}/classpath-build")
set(CLASSPATH_INSTALL_DIR "${VENDOR_PREFIX}/classpath")

# Get Java paths - we use these explicitly and clear everything else
get_filename_component(JAVA_BIN_DIR "${Java_JAVAC_EXECUTABLE}" DIRECTORY)

# Create a script that clears Java environment and runs the build
# This ensures no OpenJDK tools accidentally leak from PATH
set(CLEAN_JAVA_ENV_VARS
    "--unset=JAVA_HOME"
    "--unset=JDK_HOME"
    "--unset=CLASSPATH"
)

ExternalProject_Add(gnu_classpath
    SOURCE_DIR "${CLASSPATH_SOURCE_DIR}"
    BINARY_DIR "${CLASSPATH_BUILD_DIR}"
    INSTALL_DIR "${CLASSPATH_INSTALL_DIR}"

    # Clean Java environment and explicitly set only what we need
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        "JAVAC=${Java_JAVAC_EXECUTABLE}"
        "JAVA=${Java_JAVA_EXECUTABLE}"
        "JAR=${JAVA_BIN_DIR}/jar"
        "ECJ="
        "${CLASSPATH_SOURCE_DIR}/configure"
            --prefix=<INSTALL_DIR>
            --disable-gtk-peer
            --disable-gconf-peer
            --disable-plugin
            --disable-examples
            --disable-Werror
            --disable-gjdoc

    BUILD_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        "JAVAC=${Java_JAVAC_EXECUTABLE}"
        "JAVA=${Java_JAVA_EXECUTABLE}"
        "JAR=${JAVA_BIN_DIR}/jar"
        ${MAKE_COMMAND} -j${NPROC}

    INSTALL_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        ${MAKE_COMMAND} install

    BUILD_BYPRODUCTS "${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip"
)

set(CLASSPATH_GLIBJ_ZIP "${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip" PARENT_SCOPE)

# ============================================================================
# JamVM Build
# ============================================================================
set(JAMVM_BUILD_DIR "${CMAKE_BINARY_DIR}/jamvm-build")
set(JAMVM_INSTALL_DIR "${VENDOR_PREFIX}/jamvm")

# Use JOPA to rebuild JamVM's classes.zip from source for a fair no-blob bootstrap
# The JAVAC variable is used by JamVM's Makefile to compile Java sources
set(JOPA_EXECUTABLE "${CMAKE_BINARY_DIR}/src/jopa")

# Delete prebuilt classes.zip to force rebuild from source using JOPA
set(JAMVM_PREBUILT_CLASSES "${JAMVM_SOURCE_DIR}/src/classlib/gnuclasspath/lib/classes.zip")

ExternalProject_Add(jamvm
    SOURCE_DIR "${JAMVM_SOURCE_DIR}"
    BINARY_DIR "${JAMVM_BUILD_DIR}"
    INSTALL_DIR "${JAMVM_INSTALL_DIR}"

    DEPENDS gnu_classpath

    # Delete prebuilt classes.zip, then configure with JOPA as compiler
    # This forces JamVM's Makefile to rebuild classes.zip from source
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E rm -f "${JAMVM_PREBUILT_CLASSES}"
    COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        "JAVAC=${JOPA_EXECUTABLE} -source 1.5 -target 1.5"
        "${JAMVM_SOURCE_DIR}/configure"
            --prefix=<INSTALL_DIR>
            --with-java-runtime-library=gnuclasspath
            --with-classpath-install-dir=${CLASSPATH_INSTALL_DIR}
            --enable-int-inlining

    BUILD_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        "JAVAC=${JOPA_EXECUTABLE} -source 1.5 -target 1.5"
        ${MAKE_COMMAND} -j${NPROC}

    INSTALL_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        ${MAKE_COMMAND} install

    BUILD_BYPRODUCTS "${JAMVM_INSTALL_DIR}/bin/jamvm"
)

set(JAMVM_EXECUTABLE "${JAMVM_INSTALL_DIR}/bin/jamvm" PARENT_SCOPE)
set(CLASSPATH_INSTALL_PREFIX "${CLASSPATH_INSTALL_DIR}" PARENT_SCOPE)

# vendor_jvm depends on both jamvm and jopa (since jamvm uses JOPA to compile classes.zip)
add_custom_target(vendor_jvm DEPENDS jamvm)

# Ensure jamvm waits for jopa to be built before starting its build
# (ExternalProject can't directly depend on regular targets, so we add a step)
ExternalProject_Add_StepDependencies(jamvm configure jopa)

message(STATUS "JamVM: ${JAMVM_SOURCE_DIR}")
message(STATUS "GNU Classpath: ${CLASSPATH_SOURCE_DIR}")
message(STATUS "Install prefix: ${VENDOR_PREFIX}")
