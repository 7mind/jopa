cmake_minimum_required(VERSION 3.20)

# Default to Debug build if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type (Debug/Release)" FORCE)
    message(STATUS "Build type not specified, defaulting to Debug")
endif()

# Use clang as the default compiler if not specified
if(NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED CMAKE_CXX_COMPILER)
    find_program(CLANG_C_COMPILER clang)
    find_program(CLANG_CXX_COMPILER clang++)
    if(CLANG_C_COMPILER AND CLANG_CXX_COMPILER)
        set(CMAKE_C_COMPILER "${CLANG_C_COMPILER}" CACHE STRING "C compiler")
        set(CMAKE_CXX_COMPILER "${CLANG_CXX_COMPILER}" CACHE STRING "C++ compiler")
    endif()
endif()

# Read version from flake.nix (before project() so we can use it there)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/flake.nix" FLAKE_NIX_CONTENT)
string(REGEX MATCH "version = \"([0-9]+\\.[0-9]+)" _ "${FLAKE_NIX_CONTENT}")
set(JOPA_VERSION "${CMAKE_MATCH_1}")
if(NOT JOPA_VERSION)
    message(FATAL_ERROR "Could not extract version from flake.nix")
endif()
message(STATUS "JOPA version: ${JOPA_VERSION}")

project(jopa VERSION ${JOPA_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Determine if this is a debug build
string(TOUPPER "${CMAKE_BUILD_TYPE}" _BUILD_TYPE_UPPER)
if(_BUILD_TYPE_UPPER STREQUAL "DEBUG")
    set(_IS_DEBUG_BUILD TRUE)
else()
    set(_IS_DEBUG_BUILD FALSE)
endif()

option(JOPA_ENABLE_DEBUG "Enable internal compiler debugging traces" OFF)
option(JOPA_ENABLE_NATIVE_FP "Use native floating point instead of emulation" ON)
option(JOPA_ENABLE_ENCODING "Enable -encoding support via iconv or ICU" ON)
option(JOPA_ENABLE_CPPTRACE "Enable cpptrace for better stack traces on crashes" ${_IS_DEBUG_BUILD})
option(JOPA_ENABLE_SANITIZERS "Enable Clang sanitizers (ASan, UBSan)" ${_IS_DEBUG_BUILD})
option(JOPA_ENABLE_LEAK_SANITIZER "Enable memory leak detection (requires JOPA_ENABLE_SANITIZERS)" ${_IS_DEBUG_BUILD})

# JamVM bootstrap options
option(JOPA_BUILD_JAMVM "Build JamVM + GNU Classpath for bootstrap testing" ON)
option(JOPA_USE_JAMVM_TESTS "Run tests with JamVM instead of system JVM" OFF)

set(JOPA_VERSION_STRING "${JOPA_VERSION}")
set(PACKAGE_NAME "jopa")
set(PACKAGE_TARNAME "${PACKAGE_NAME}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${JOPA_VERSION}")

include(GNUInstallDirs)
include(CTest)

# Build jikespg (parser generator) - needed for grammar generation
add_subdirectory(vendor/jikespg)

# Build JamVM + GNU Classpath if requested
if(JOPA_BUILD_JAMVM)
    add_subdirectory(vendor)
endif()

add_subdirectory(src)

# Runtime library (needed for tests)
add_subdirectory(runtime)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(FILES doc/jikes.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DOCDIR}")

# =============================================================================
# DevJopaK - Self-contained Java Development Kit distribution
# =============================================================================
# Creates a distribution archive with JOPA compiler, JamVM runtime, and GNU Classpath.
# This is a "no-blob bootstrap" kit - all Java bytecode is compiled from source using JOPA.
#
# Usage: cmake --build . --target devjopak
#
# The resulting archive contains:
#   bin/javac    - JOPA compiler wrapper
#   bin/java     - JamVM runtime wrapper
#   lib/jopa     - JOPA compiler binary
#   lib/jamvm    - JamVM runtime binary
#   lib/classes.zip - JamVM bootstrap classes (compiled by JOPA)
#   lib/glibj.zip   - GNU Classpath runtime library
#
if(JOPA_BUILD_JAMVM)
    set(DEVJOPAK_DIR "${CMAKE_BINARY_DIR}/devjopak")
    set(DEVJOPAK_ARCHIVE "devjopak-${JOPA_VERSION}.tar.gz")

    # Create wrapper scripts
    set(JAVAC_WRAPPER "${DEVJOPAK_DIR}/bin/javac")
    set(JAVA_WRAPPER "${DEVJOPAK_DIR}/bin/java")

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEVJOPAK_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEVJOPAK_DIR}/lib"
        COMMAND ${CMAKE_COMMAND} -E touch "${DEVJOPAK_DIR}/bin/.created"
        COMMENT "Creating devjopak directory structure"
    )

    add_custom_command(
        OUTPUT "${JAVAC_WRAPPER}"
        DEPENDS "${DEVJOPAK_DIR}/bin/.created" jopa "${CMAKE_SOURCE_DIR}/scripts/devjopak-javac.sh.in"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/scripts/devjopak-javac.sh.in" "${JAVAC_WRAPPER}"
        COMMAND chmod +x "${JAVAC_WRAPPER}"
        COMMENT "Creating javac wrapper script"
    )

    add_custom_command(
        OUTPUT "${JAVA_WRAPPER}"
        DEPENDS "${DEVJOPAK_DIR}/bin/.created" jamvm_with_gnucp "${CMAKE_SOURCE_DIR}/scripts/devjopak-java.sh.in"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/scripts/devjopak-java.sh.in" "${JAVA_WRAPPER}"
        COMMAND chmod +x "${JAVA_WRAPPER}"
        COMMENT "Creating java wrapper script"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/jopa"
        DEPENDS jopa "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:jopa>" "${DEVJOPAK_DIR}/lib/jopa"
        COMMENT "Copying JOPA compiler"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/jamvm"
        DEPENDS jamvm_with_gnucp "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/vendor-install/jamvm/bin/jamvm" "${DEVJOPAK_DIR}/lib/jamvm"
        COMMENT "Copying JamVM runtime"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/classes.zip"
        DEPENDS jamvm_with_gnucp "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/vendor-install/jamvm/share/jamvm/classes.zip" "${DEVJOPAK_DIR}/lib/classes.zip"
        COMMENT "Copying JamVM classes (compiled by JOPA)"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/glibj.zip"
        DEPENDS gnu_classpath "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/vendor-install/classpath/share/classpath/glibj.zip" "${DEVJOPAK_DIR}/lib/glibj.zip"
        COMMENT "Copying GNU Classpath runtime"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/lib/native/.created"
        DEPENDS gnu_classpath "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEVJOPAK_DIR}/lib/native"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_BINARY_DIR}/vendor-install/classpath/lib/classpath"
            "${DEVJOPAK_DIR}/lib/native"
        COMMAND ${CMAKE_COMMAND} -E touch "${DEVJOPAK_DIR}/lib/native/.created"
        COMMENT "Copying GNU Classpath native libraries"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_DIR}/LICENSE"
        DEPENDS "${DEVJOPAK_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_SOURCE_DIR}/LICENSE" "${DEVJOPAK_DIR}/LICENSE"
        COMMENT "Copying LICENSE"
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
            "${DEVJOPAK_DIR}/LICENSE"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E tar czf "${DEVJOPAK_ARCHIVE}" "devjopak"
        COMMENT "Creating devjopak distribution archive"
    )

    add_custom_target(devjopak
        DEPENDS "${CMAKE_BINARY_DIR}/${DEVJOPAK_ARCHIVE}"
        COMMENT "DevJopaK - Self-contained Java Development Kit (no-blob bootstrap)"
    )

    message(STATUS "DevJopaK target available: cmake --build . --target devjopak")
endif()
