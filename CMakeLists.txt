cmake_minimum_required(VERSION 3.20)

project(jikes VERSION 1.22 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(JIKES_ENABLE_DEBUG "Enable internal compiler debugging traces" OFF)
option(JIKES_ENABLE_NATIVE_FP "Use native floating point instead of emulation" ON)
option(JIKES_ENABLE_ENCODING "Enable -encoding support via iconv or ICU" ON)

set(JIKES_VERSION_STRING "Version 1.22 - 3 October 2004")
set(PACKAGE_NAME "jikes")
set(PACKAGE_TARNAME "${PACKAGE_NAME}")
set(PACKAGE_STRING "${PACKAGE_NAME} ${PROJECT_VERSION}")

include(GNUInstallDirs)
include(CTest)

add_subdirectory(src)

# Runtime library (needed for tests)
add_subdirectory(runtime)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(FILES doc/jikes.1 DESTINATION "${CMAKE_INSTALL_MANDIR}/man1")
install(FILES LICENSE DESTINATION "${CMAKE_INSTALL_DOCDIR}")
