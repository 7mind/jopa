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
# Use wrapper script that closes stdin to prevent zip from hanging on clean builds
set(ZIP_WRAPPER "${CMAKE_CURRENT_SOURCE_DIR}/zip-wrapper.sh")

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
set(JOPA_CLASSPATH_VERSION "0.99" CACHE STRING "GNU Classpath version to use (0.93 or 0.99)")
set(CLASSPATH_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/classpath-${JOPA_CLASSPATH_VERSION}.tar.gz")
set(JAMVM_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/jamvm-2.0.0.tar.gz")
set(CLASSPATH_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/classpath-${JOPA_CLASSPATH_VERSION}")
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
    # Patch Makefile.in to remove > /dev/null so we can see zip output for debugging
    execute_process(
        COMMAND sed -i -e "s|> /dev/null||g" "${CLASSPATH_SOURCE_DIR}/lib/Makefile.in"
        RESULT_VARIABLE SED_RESULT
    )
    # Patch native/fdlibm/dtoa.c to include stdlib.h (for free())
    execute_process(
        COMMAND sed -i -e "/#include <string.h>/a #include <stdlib.h>" "${CLASSPATH_SOURCE_DIR}/native/fdlibm/dtoa.c"
        RESULT_VARIABLE SED_RESULT_DTOA
    )
endif()

# Extract JamVM if not present (only if not building legacy Classpath 0.93)
if(NOT JOPA_CLASSPATH_VERSION STREQUAL "0.93")
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

# Determine configuration flags and environment based on version
if(JOPA_CLASSPATH_VERSION VERSION_LESS "0.98")
    # Older classpath requires explicit compiler flag like --with-jikes
    # and expects JIKES environment variable
    set(CLASSPATH_EXTRA_CONF_FLAGS "--with-jikes")
    set(CLASSPATH_COMPILER_ENV
        "JIKES=${JOPA_EXECUTABLE} --nowarn:unchecked -bootclasspath ${JOPA_RUNTIME_JAR}"
    )
else()
    # Newer classpath (0.99) respects JAVAC environment variable
    set(CLASSPATH_EXTRA_CONF_FLAGS "")
    set(CLASSPATH_COMPILER_ENV
        "JAVAC=${JOPA_EXECUTABLE} --nowarn:unchecked -bootclasspath ${JOPA_RUNTIME_JAR}"
        "ECJ="
    )
endif()

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
        ${CLASSPATH_COMPILER_ENV}
        "ZIP=${ZIP_WRAPPER}"
        "${CLASSPATH_SOURCE_DIR}/configure"
            --prefix=<INSTALL_DIR>
            --with-jar=no
            --disable-gtk-peer
            --disable-gconf-peer
            --disable-plugin
            --disable-examples
            --disable-Werror
            --disable-gjdoc
            ${CLASSPATH_EXTRA_CONF_FLAGS}

    # Note: Using -j1 for GNU Classpath build because JOPA is single-threaded
    # and parallel make causes race conditions with the glibj.zip creation
    # V=1 enables verbose mode to show actual commands being executed (useful for CI debugging)
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

# GNU Classpath depends on JOPA and runtime.jar being built first
ExternalProject_Add_StepDependencies(gnu_classpath configure jopa jopa-stub-rt)

set(CLASSPATH_GLIBJ_ZIP "${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip")
set(CLASSPATH_GLIBJ_ZIP "${CLASSPATH_GLIBJ_ZIP}" PARENT_SCOPE)

# ============================================================================
# JamVM Build - classes.zip compiled with JOPA
# ============================================================================
if(NOT JOPA_CLASSPATH_VERSION STREQUAL "0.93")
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

    set(JAMVM_EXECUTABLE "${JAMVM_INSTALL_DIR}/bin/jamvm")
    set(JAMVM_EXECUTABLE "${JAMVM_EXECUTABLE}" PARENT_SCOPE)
    set(CLASSPATH_INSTALL_PREFIX "${CLASSPATH_INSTALL_DIR}")
    set(CLASSPATH_INSTALL_PREFIX "${CLASSPATH_INSTALL_PREFIX}" PARENT_SCOPE)

    # jamvm_with_gnucp is the main target for building the complete JVM stack
    add_custom_target(jamvm_with_gnucp DEPENDS jamvm)

    message(STATUS "Bootstrap: ALL Java bytecode will be compiled by JOPA")
    message(STATUS "JamVM sources: ${JAMVM_SOURCE_DIR}")
endif()

message(STATUS "GNU Classpath sources: ${CLASSPATH_SOURCE_DIR}")
message(STATUS "Install prefix: ${VENDOR_PREFIX}")

# ============================================================================
# Apache Ant Build (1.8.4) - Built with JOPA/JamVM
# ============================================================================
# Only build Ant if we have a working JamVM (not legacy classpath 0.93)
if(NOT JOPA_CLASSPATH_VERSION STREQUAL "0.93")
    set(ANT_VERSION "1.8.4")
    set(ANT_ARCHIVE "${CMAKE_CURRENT_SOURCE_DIR}/apache-ant-${ANT_VERSION}-src.tar.gz")
    set(ANT_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/apache-ant-${ANT_VERSION}")
    set(ANT_INSTALL_DIR "${VENDOR_PREFIX}/ant")
    
    # Define wrapper scripts for Ant build (using build artifacts directly)
    # We cannot rely on devjopak wrappers because devjopak target is defined after this file.
    set(ANT_BUILD_JAVAC "${JOPA_EXECUTABLE} -bootclasspath ${CLASSPATH_GLIBJ_ZIP}")
    
    # JamVM needs generic flags to find GNU Classpath
    # Note: We must use the build-tree paths, not install-tree, unless we installed.
    # jamvm_with_gnucp installs to vendor-install, so we use that.
    set(ANT_BUILD_JAVA "${JAMVM_EXECUTABLE} -Dgnu.classpath.boot.library.path=${VENDOR_PREFIX}/classpath/lib/classpath -Xbootclasspath:${CLASSPATH_GLIBJ_ZIP}")

    # Generate wrapper scripts at configure time to avoid shell escaping hell
    # We add debug output to see what's happening
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ant-javac.sh" 
"#!/bin/sh
echo \"ANT_JAVAC args: $@\" >&2
ls -l \"${CLASSPATH_GLIBJ_ZIP}\" >&2
\"${JOPA_EXECUTABLE}\" -source 1.6 -target 1.6 -bootclasspath \"${CLASSPATH_GLIBJ_ZIP}\" \"$@\"
")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ant-java.sh" 
"#!/bin/sh
ulimit -s unlimited
echo \"ANT_JAVA args: $@\" >&2
\"${JAMVM_EXECUTABLE}\" -verbose:class -Xms64m -Xmx512m -Xss16m -Dgnu.classpath.boot.library.path=\"${VENDOR_PREFIX}/classpath/lib/classpath\" -Dbuild.compiler=extJavac \"$@\"
")

    # Make them executable
    execute_process(COMMAND chmod +x "${CMAKE_CURRENT_BINARY_DIR}/ant-javac.sh" "${CMAKE_CURRENT_BINARY_DIR}/ant-java.sh")

    # --- JUnit 3.8.2 (Source Build) ---
    ExternalProject_Add(junit
        URL "https://repo1.maven.org/maven2/junit/junit/3.8.2/junit-3.8.2-sources.jar"
        PREFIX "${CMAKE_CURRENT_BINARY_DIR}/junit-prefix"
        INSTALL_DIR "${VENDOR_PREFIX}/junit"
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
        BINARY_DIR "${ANT_SOURCE_DIR}" # Build in-source
        INSTALL_DIR "${ANT_INSTALL_DIR}"
        
        DEPENDS jamvm_with_gnucp junit # Ensure tools are built

        # Configure is a no-op for Ant bootstrap
        CONFIGURE_COMMAND ""

        # Bootstrap Ant using JOPA and JamVM
        # We set CLASSPATH to empty to avoid system pollution
        # JAVAC and JAVACMD point to our wrappers
        BUILD_COMMAND ${CMAKE_COMMAND} -E make_directory lib/optional &&
                      ${CMAKE_COMMAND} -E copy "${VENDOR_PREFIX}/junit/lib/junit.jar" lib/optional/junit.jar &&
                      ${CMAKE_COMMAND} -E env
            ${CLEAN_JAVA_ENV_VARS}
            "JAVAC=${CMAKE_CURRENT_BINARY_DIR}/ant-javac.sh"
            "JAVACMD=${CMAKE_CURRENT_BINARY_DIR}/ant-java.sh"
            "CLASSPATH="
            "ANT_HOME="
            "./bootstrap.sh"

        # "Install" by copying the built dist directory
        INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory "${ANT_INSTALL_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ANT_SOURCE_DIR}/bootstrap" "${ANT_INSTALL_DIR}"
        
        BUILD_BYPRODUCTS "${ANT_INSTALL_DIR}/bin/ant"
    )
endif()
