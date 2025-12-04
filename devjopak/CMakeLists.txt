cmake_minimum_required(VERSION 3.20)

project(devjopak NONE)

# Read version from flake.nix
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../flake.nix" FLAKE_NIX_CONTENT)
string(REGEX MATCH "version = \"([0-9]+\\.[0-9]+)" _ "${FLAKE_NIX_CONTENT}")
set(JOPA_VERSION "${CMAKE_MATCH_1}")
if(NOT JOPA_VERSION)
    message(FATAL_ERROR "Could not extract version from flake.nix")
endif()
message(STATUS "JOPA version: ${JOPA_VERSION}")

# Configuration for JOPA location
set(JOPA_BUILD_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build" CACHE PATH "Path to the main JOPA build directory")
set(JOPA_EXECUTABLE "${JOPA_BUILD_DIR}/src/jopa" CACHE FILEPATH "Path to JOPA executable")
set(JOPA_RUNTIME_JAR "${JOPA_BUILD_DIR}/runtime/jopa-stub-rt.jar" CACHE FILEPATH "Path to JOPA stub runtime JAR")
# Optional wrapper for memory tracking (defaults to JOPA_EXECUTABLE)
set(JOPA_JAVAC_COMMAND "${JOPA_EXECUTABLE}" CACHE STRING "Command to use as JAVAC (can be a wrapper script)")

if(NOT EXISTS "${JOPA_EXECUTABLE}")
    message(FATAL_ERROR "JOPA executable not found at ${JOPA_EXECUTABLE}. Please build jopa first or set JOPA_EXECUTABLE.")
endif()

if(NOT EXISTS "${JOPA_RUNTIME_JAR}")
    message(FATAL_ERROR "JOPA runtime JAR not found at ${JOPA_RUNTIME_JAR}. Please build jopa first or set JOPA_RUNTIME_JAR.")
endif()

include(ExternalProject)
include(ProcessorCount)
ProcessorCount(NPROC)
if(NPROC EQUAL 0)
    set(NPROC 1)
endif()

find_program(MAKE_COMMAND make REQUIRED)
set(ZIP_WRAPPER "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/zip-wrapper.sh")

set(VENDOR_PREFIX "${CMAKE_BINARY_DIR}/vendor-install")
file(MAKE_DIRECTORY "${VENDOR_PREFIX}")

# Clean Java environment
set(CLEAN_JAVA_ENV_VARS
    "--unset=JAVA_HOME"
    "--unset=JDK_HOME"
    "--unset=CLASSPATH"
    "--unset=JAVA"
    "--unset=JAVAC"
    "--unset=JAR"
)

set(JOPA_CLASSPATH_VERSION "0.99" CACHE STRING "GNU Classpath version to use (0.93 or 0.99)")
set(CLASSPATH_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/classpath-${JOPA_CLASSPATH_VERSION}.tar.gz")
set(JAMVM_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/jamvm-2.0.0.tar.gz")
set(ANT_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/apache-ant-1.8.4-src.tar.gz")

set(CLASSPATH_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/classpath-${JOPA_CLASSPATH_VERSION}")
set(JAMVM_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/jamvm-2.0.0")

# ============================================================================ 
# GNU Classpath Build
# ============================================================================ 
set(CLASSPATH_BUILD_DIR "${CMAKE_BINARY_DIR}/classpath-${JOPA_CLASSPATH_VERSION}-build")
set(CLASSPATH_INSTALL_DIR "${VENDOR_PREFIX}/classpath")

if(JOPA_CLASSPATH_VERSION VERSION_LESS "0.98")
    set(CLASSPATH_EXTRA_CONF_FLAGS "--with-jikes")
    set(CLASSPATH_COMPILER_ENV
        "JIKES=${JOPA_JAVAC_COMMAND} --nowarn:unchecked -bootclasspath ${JOPA_RUNTIME_JAR}"
    )
else()
    set(CLASSPATH_EXTRA_CONF_FLAGS "")
    set(CLASSPATH_COMPILER_ENV
        "JAVAC=${JOPA_JAVAC_COMMAND} --nowarn:unchecked -bootclasspath ${JOPA_RUNTIME_JAR}"
        "ECJ="
    )
endif()

ExternalProject_Add(gnu_classpath
    URL "${CLASSPATH_ARCHIVE}"
    SOURCE_DIR "${CLASSPATH_SOURCE_DIR}"
    BINARY_DIR "${CLASSPATH_BUILD_DIR}"
    INSTALL_DIR "${CLASSPATH_INSTALL_DIR}"
    
    PATCH_COMMAND sed -i -e "/#include <string.h>/a #include <stdlib.h>" <SOURCE_DIR>/native/fdlibm/dtoa.c
    COMMAND patch -p0 -d <SOURCE_DIR> < ${CMAKE_CURRENT_SOURCE_DIR}/../vendor/classpath-string-valueof.patch

    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        "UBSAN_OPTIONS=halt_on_error=0"
        "ASAN_OPTIONS=detect_leaks=0"
        ${CLASSPATH_COMPILER_ENV}
        "ZIP=${ZIP_WRAPPER}"
        <SOURCE_DIR>/configure
            --prefix=<INSTALL_DIR>
            --with-jar=no
            --disable-gtk-peer
            --disable-gconf-peer
            --disable-plugin
            --disable-examples
            --disable-Werror
            --disable-gjdoc
            ${CLASSPATH_EXTRA_CONF_FLAGS}

    BUILD_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        "UBSAN_OPTIONS=halt_on_error=0"
        "ASAN_OPTIONS=detect_leaks=0"
        ${CLASSPATH_COMPILER_ENV}
        "ZIP=${ZIP_WRAPPER}"
        ${MAKE_COMMAND} -j1 V=1

    INSTALL_COMMAND ${CMAKE_COMMAND} -E env
        ${CLEAN_JAVA_ENV_VARS}
        ${MAKE_COMMAND} install

    BUILD_BYPRODUCTS "${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip"
)

set(CLASSPATH_GLIBJ_ZIP "${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip")

# ============================================================================ 
# JamVM Build
# ============================================================================ 
if(NOT JOPA_CLASSPATH_VERSION STREQUAL "0.93")
    set(JAMVM_BUILD_DIR "${CMAKE_BINARY_DIR}/jamvm-classpath-${JOPA_CLASSPATH_VERSION}-build")
    set(JAMVM_INSTALL_DIR "${VENDOR_PREFIX}/jamvm")
    set(JAMVM_PREBUILT_CLASSES "${JAMVM_SOURCE_DIR}/src/classlib/gnuclasspath/lib/classes.zip")

    ExternalProject_Add(jamvm
        URL "${JAMVM_ARCHIVE}"
        SOURCE_DIR "${JAMVM_SOURCE_DIR}"
        BINARY_DIR "${JAMVM_BUILD_DIR}"
        INSTALL_DIR "${JAMVM_INSTALL_DIR}"

        DEPENDS gnu_classpath

        CONFIGURE_COMMAND ${CMAKE_COMMAND} -E rm -f "${JAMVM_PREBUILT_CLASSES}"
        COMMAND ${CMAKE_COMMAND} -E env
            ${CLEAN_JAVA_ENV_VARS}
            "UBSAN_OPTIONS=halt_on_error=0"
            "ASAN_OPTIONS=detect_leaks=0"
            "JAVAC=${JOPA_JAVAC_COMMAND} --nowarn:unchecked -bootclasspath ${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip"
            <SOURCE_DIR>/configure
                --prefix=<INSTALL_DIR>
                --with-java-runtime-library=gnuclasspath
                --with-classpath-install-dir=${CLASSPATH_INSTALL_DIR}
                --enable-int-inlining

        BUILD_COMMAND ${CMAKE_COMMAND} -E env
            ${CLEAN_JAVA_ENV_VARS}
            "UBSAN_OPTIONS=halt_on_error=0"
            "ASAN_OPTIONS=detect_leaks=0"
            "JAVAC=${JOPA_JAVAC_COMMAND} --nowarn:unchecked -bootclasspath ${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip"
            ${MAKE_COMMAND} -j${NPROC}

        INSTALL_COMMAND ${CMAKE_COMMAND} -E env
            ${CLEAN_JAVA_ENV_VARS}
            ${MAKE_COMMAND} install

        BUILD_BYPRODUCTS "${JAMVM_INSTALL_DIR}/bin/jamvm"
    )

    set(JAMVM_EXECUTABLE "${JAMVM_INSTALL_DIR}/bin/jamvm")
    add_custom_target(jamvm_with_gnucp DEPENDS jamvm)
endif()

# ============================================================================ 
# Apache Ant Build
# ============================================================================ 
if(NOT JOPA_CLASSPATH_VERSION STREQUAL "0.93")
    set(ANT_VERSION "1.8.4")
    set(ANT_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/apache-ant-${ANT_VERSION}")
    set(ANT_INSTALL_DIR "${VENDOR_PREFIX}/ant")
    
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ant-javac.sh"
"#!/bin/sh
\"${JOPA_JAVAC_COMMAND}\" -source 1.6 -target 1.6 -bootclasspath \"${CLASSPATH_GLIBJ_ZIP}\" \"$@\"
")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ant-java.sh" 
"#!/bin/sh
ulimit -s unlimited
\"${JAMVM_EXECUTABLE}\" -Xms64m -Xmx512m -Xss16m -Dgnu.classpath.boot.library.path=\"${VENDOR_PREFIX}/classpath/lib/classpath\" -Dbuild.compiler=extJavac \"$@\" 
")
    execute_process(COMMAND chmod +x "${CMAKE_CURRENT_BINARY_DIR}/ant-javac.sh" "${CMAKE_CURRENT_BINARY_DIR}/ant-java.sh")

    ExternalProject_Add(junit
        URL "https://repo1.maven.org/maven2/junit/junit/3.8.2/junit-3.8.2-sources.jar"
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/junit-prefix"
        INSTALL_DIR "${VENDOR_PREFIX}/junit"
        DEPENDS gnu_classpath
        CONFIGURE_COMMAND ""
        BUILD_COMMAND sh -c "mkdir -p bin && find junit -name '*.java' > sources.list" &&
                      "${CMAKE_CURRENT_BINARY_DIR}/ant-javac.sh" -d bin -source 1.6 -target 1.6 @sources.list &&
                      sh -c "cd bin && \"${CMAKE_COMMAND}\" -E tar cf ../junit.jar --format=zip ."
        INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${VENDOR_PREFIX}/junit/lib" &&
                        ${CMAKE_COMMAND} -E copy junit.jar "${VENDOR_PREFIX}/junit/lib/junit.jar"
        BUILD_IN_SOURCE 1
    )

    ExternalProject_Add(apache_ant
        URL "${ANT_ARCHIVE}"
        SOURCE_DIR "${ANT_SOURCE_DIR}"
        BINARY_DIR "${ANT_SOURCE_DIR}"
        INSTALL_DIR "${ANT_INSTALL_DIR}"
        
        DEPENDS jamvm_with_gnucp junit

        CONFIGURE_COMMAND ""
        BUILD_COMMAND ${CMAKE_COMMAND} -E make_directory lib/optional &&
                      ${CMAKE_COMMAND} -E copy "${VENDOR_PREFIX}/junit/lib/junit.jar" lib/optional/junit.jar &&
                      ${CMAKE_COMMAND} -E env
            ${CLEAN_JAVA_ENV_VARS}
            "JAVAC=${CMAKE_CURRENT_BINARY_DIR}/ant-javac.sh"
            "JAVACMD=${CMAKE_CURRENT_BINARY_DIR}/ant-java.sh"
            "CLASSPATH="
            "ANT_HOME="
            "./bootstrap.sh"

        INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${ANT_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ANT_SOURCE_DIR}/bootstrap" "${ANT_INSTALL_DIR}"
        
        BUILD_BYPRODUCTS "${ANT_INSTALL_DIR}/bin/ant"
    )
endif()

# ============================================================================ 
# DevJopaK Distribution
# ============================================================================ 
if(NOT JOPA_CLASSPATH_VERSION STREQUAL "0.93")
    set(DEVJOPAK_DIR "${CMAKE_BINARY_DIR}/devjopak")
    set(DEVJOPAK_ARCHIVE "devjopak-${JOPA_VERSION}.tar.gz")

    set(JAVAC_WRAPPER "${DEVJOPAK_DIR}/bin/javac")
    set(JAVA_WRAPPER "${DEVJOPAK_DIR}/bin/java")

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEVJOPAK_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEVJOPAK_DIR}/lib"
        COMMAND ${CMAKE_COMMAND} -E touch "${DEVJOPAK_DIR}/bin/.created"
    )

    add_custom_command(
        OUTPUT "${JAVAC_WRAPPER}"
        DEPENDS "${DEVJOPAK_DIR}/bin/.created" "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/devjopak-javac.sh.in"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/devjopak-javac.sh.in" "${JAVAC_WRAPPER}"
        COMMAND chmod +x "${JAVAC_WRAPPER}"
    )

    add_custom_command(
        OUTPUT "${JAVA_WRAPPER}"
        DEPENDS "${DEVJOPAK_DIR}/bin/.created" jamvm_with_gnucp "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/devjopak-java.sh.in"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/devjopak-java.sh.in" "${JAVA_WRAPPER}"
        COMMAND chmod +x "${JAVA_WRAPPER}"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/jopa"
        DEPENDS "${DEVJOPAK_DIR}/bin/.created" "${JOPA_EXECUTABLE}"
        COMMAND ${CMAKE_COMMAND} -E copy "${JOPA_EXECUTABLE}" "${DEVJOPAK_DIR}/lib/jopa"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/jamvm"
        DEPENDS jamvm_with_gnucp "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${JAMVM_INSTALL_DIR}/bin/jamvm" "${DEVJOPAK_DIR}/lib/jamvm"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/classes.zip"
        DEPENDS jamvm_with_gnucp "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${JAMVM_INSTALL_DIR}/share/jamvm/classes.zip" "${DEVJOPAK_DIR}/lib/classes.zip"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/glibj.zip"
        DEPENDS gnu_classpath "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip" "${DEVJOPAK_DIR}/lib/glibj.zip"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/native/.created"
        DEPENDS gnu_classpath "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEVJOPAK_DIR}/lib/native"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CLASSPATH_INSTALL_DIR}/lib/classpath"
            "${DEVJOPAK_DIR}/lib/native"
        COMMAND ${CMAKE_COMMAND} -E touch "${DEVJOPAK_DIR}/lib/native/.created"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/bin/ant"
        DEPENDS apache_ant "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${ANT_INSTALL_DIR}/bin/ant" "${DEVJOPAK_DIR}/bin/ant"
        COMMAND chmod +x "${DEVJOPAK_DIR}/bin/ant"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/ant.jar"
        DEPENDS apache_ant "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ANT_INSTALL_DIR}/lib" "${DEVJOPAK_DIR}/lib"
        COMMAND ${CMAKE_COMMAND} -E copy "${VENDOR_PREFIX}/junit/lib/junit.jar" "${DEVJOPAK_DIR}/lib/junit.jar"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/LICENSE"
        DEPENDS "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE" "${DEVJOPAK_DIR}/LICENSE"
    )



    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/bin/devjopak-validate"
        DEPENDS "${DEVJOPAK_DIR}/bin/.created" "${CMAKE_CURRENT_SOURCE_DIR}/devjopak-validate.sh"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/devjopak-validate.sh" "${DEVJOPAK_DIR}/bin/devjopak-validate"
        COMMAND chmod +x "${DEVJOPAK_DIR}/bin/devjopak-validate"
    )

    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/${DEVJOPAK_ARCHIVE}"
        DEPENDS
            "${JAVAC_WRAPPER}"
            "${JAVA_WRAPPER}"
            "${DEVJOPAK_DIR}/lib/jopa"
            "${DEVJOPAK_DIR}/lib/jamvm"
            "${DEVJOPAK_DIR}/lib/classes.zip"
            "${DEVJOPAK_DIR}/lib/glibj.zip"
            "${DEVJOPAK_DIR}/lib/native/.created"
            "${DEVJOPAK_DIR}/bin/ant"
            "${DEVJOPAK_DIR}/lib/ant.jar"
            "${DEVJOPAK_DIR}/LICENSE"
            "${DEVJOPAK_DIR}/bin/devjopak-validate" # Add dependency for the validation script
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E tar czf "${DEVJOPAK_ARCHIVE}" "devjopak"
    )

    add_custom_target(devjopak
        DEPENDS "${CMAKE_BINARY_DIR}/${DEVJOPAK_ARCHIVE}"
        COMMENT "DevJopaK - Self-contained Java Development Kit"
    )

    include(ecj_build.cmake)
endif()
