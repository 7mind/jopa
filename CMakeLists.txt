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
option(JOPA_ENABLE_LEAK_SANITIZER "Enable memory leak detection (requires JOPA_ENABLE_SANITIZERS)" OFF)

set(JOPA_VERSION_STRING "${JOPA_VERSION}")
set(PACKAGE_NAME "jopa")
set(PACKAGE_TARNAME "${PACKAGE_NAME}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${JOPA_VERSION}")

include(GNUInstallDirs)
include(CTest)

# Build jikespg (parser generator) - needed for grammar generation
add_subdirectory(vendor/jikespg)

add_subdirectory(src)

# Runtime library (needed for tests)
add_subdirectory(runtime)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(FILES doc/jikes.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DOCDIR}")

# Installation Target
install(TARGETS jopa DESTINATION bin)
