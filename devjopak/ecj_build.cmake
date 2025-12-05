# ============================================================================
# Eclipse Compiler for Java (ECJ) Build
# ============================================================================
if(NOT JOPA_CLASSPATH_VERSION STREQUAL "0.93")
    # ECJ version - can be overridden on command line
    set(ECJ_VERSION "4.2.1" CACHE STRING "ECJ version to use (4.2.1 or 4.2.2)")
    # Use local source JAR from vendor directory
    set(ECJ_SOURCE_JAR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/ecjsrc-${ECJ_VERSION}.jar")

    if(NOT EXISTS "${ECJ_SOURCE_JAR}")
        message(FATAL_ERROR "ECJ source JAR not found at ${ECJ_SOURCE_JAR}. Available versions: 4.2.1, 4.2.2")
    endif()
    message(STATUS "ECJ version: ${ECJ_VERSION}")
    
    set(ECJ_BUILD_DIR "${CMAKE_BINARY_DIR}/ecj-build")
    set(ECJ_INSTALL_DIR "${VENDOR_PREFIX}/ecj")
    set(ECJ_JAR "${ECJ_INSTALL_DIR}/lib/ecj.jar")

    # Script to generate sources list safely
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gen_ecj_sources.sh"
"#!/bin/sh
find \"$1\" -name '*.java' > \"$2\"
")

    add_custom_command(
        OUTPUT "${ECJ_JAR}"
        DEPENDS jamvm_with_gnucp "${ECJ_SOURCE_JAR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ECJ_BUILD_DIR}/classes"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ECJ_INSTALL_DIR}/lib"
        
        # 1. Prepare source directory
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${ECJ_BUILD_DIR}/src"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ECJ_BUILD_DIR}/src"
        
        # 2. Extract source JAR to build directory
        # We use 'unzip' if available or cmake -E tar (which supports zip in newer cmake, but might be flaky on some old versions)
        # Let's try cmake -E tar xf first as it is portable
        COMMAND ${CMAKE_COMMAND} -E chdir "${ECJ_BUILD_DIR}/src" ${CMAKE_COMMAND} -E tar xf "${ECJ_SOURCE_JAR}"
        
        # 3. Remove problematic sources that depend on missing APIs (javax.tools, javax.annotation.processing)
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${ECJ_BUILD_DIR}/src/org/eclipse/jdt/internal/compiler/tool"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${ECJ_BUILD_DIR}/src/org/eclipse/jdt/internal/compiler/apt"
        COMMAND ${CMAKE_COMMAND} -E remove "${ECJ_BUILD_DIR}/src/org/eclipse/jdt/core/JDTCompilerAdapter.java"
        
        # 4. Compile the remaining sources
        # We use a helper script to generate the list of files to avoid shell quoting issues in CMake
        COMMAND sh "${CMAKE_CURRENT_BINARY_DIR}/gen_ecj_sources.sh" "${ECJ_BUILD_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}/ecj_sources.list"
        
        COMMAND ${CMAKE_COMMAND} -E env
            ${CLEAN_JAVA_ENV_VARS}
            "${CMAKE_CURRENT_BINARY_DIR}/ant-javac.sh"
            -d "${ECJ_BUILD_DIR}/classes"
            -source 1.5 -target 1.5
            -encoding ISO-8859-1
            -nowarn
            "@${CMAKE_CURRENT_BINARY_DIR}/ecj_sources.list"
            
        # 5. Copy resources (properties, etc.) to classes dir
        # We reuse the source tree for this, identifying non-java files
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ECJ_BUILD_DIR}/src" "${ECJ_BUILD_DIR}/classes"
        COMMAND find "${ECJ_BUILD_DIR}/classes" -name "*.java" -delete

        # 6. Package into JAR
        COMMAND ${CMAKE_COMMAND} -E chdir "${ECJ_BUILD_DIR}/classes" "${ZIP_WRAPPER}" -r "${ECJ_JAR}" .
        
        COMMENT "Building Eclipse Compiler for Java (ECJ) ${ECJ_VERSION}"
    )

    add_custom_target(ecj DEPENDS "${ECJ_JAR}")

    # ============================================================================
    # DevJopaK-ECJ Distribution
    # ============================================================================
    # Tarball naming: devjopak-{jopa_version}-gnucp-{cp_version}-ecj-{ecj_version}
    set(DEVJOPAK_ECJ_DIR "${CMAKE_BINARY_DIR}/devjopak-ecj")
    set(DEVJOPAK_ECJ_ARCHIVE "devjopak-${JOPA_VERSION}-gnucp-${JOPA_CLASSPATH_VERSION}-ecj-${ECJ_VERSION}.tar.gz")
    set(JAVAC_ECJ_WRAPPER "${DEVJOPAK_ECJ_DIR}/bin/javac")
    
    add_custom_command(
        OUTPUT "${DEVJOPAK_ECJ_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEVJOPAK_ECJ_DIR}/bin"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEVJOPAK_ECJ_DIR}/lib"
        COMMAND ${CMAKE_COMMAND} -E touch "${DEVJOPAK_ECJ_DIR}/bin/.created"
    )

    # Wrapper script for ECJ
    # We need to create this script
    set(JAVAC_ECJ_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/javac-ecj.sh")
    file(WRITE "${JAVAC_ECJ_SCRIPT}" 
"#!/bin/sh
# DevJopaK-ECJ javac wrapper
DEVJOPAK_HOME=\"$(cd \"$(dirname \"$0\")/..\" && pwd)\"
export LD_LIBRARY_PATH=\"$DEVJOPAK_HOME/lib/native:$LD_LIBRARY_PATH\"
unset CLASSPATH
exec \"$DEVJOPAK_HOME/bin/java\" \
    -cp \"$DEVJOPAK_HOME/lib/ecj.jar\" \
    org.eclipse.jdt.internal.compiler.batch.Main \
    -bootclasspath \"$DEVJOPAK_HOME/lib/classes.zip:$DEVJOPAK_HOME/lib/glibj.zip\" \
    \"$@\"
")

    add_custom_command(
        OUTPUT "${JAVAC_ECJ_WRAPPER}"
        DEPENDS "${DEVJOPAK_ECJ_DIR}/bin/.created" "${JAVAC_ECJ_SCRIPT}"
        COMMAND ${CMAKE_COMMAND} -E copy "${JAVAC_ECJ_SCRIPT}" "${JAVAC_ECJ_WRAPPER}"
        COMMAND chmod +x "${JAVAC_ECJ_WRAPPER}"
    )

    # Re-use other components from standard DevJopaK
    # We can reuse the java wrapper and libs
    
    add_custom_command(
        OUTPUT "${DEVJOPAK_ECJ_DIR}/bin/java"
        DEPENDS "${DEVJOPAK_ECJ_DIR}/bin/.created" "${JAVA_WRAPPER}"
        COMMAND ${CMAKE_COMMAND} -E copy "${JAVA_WRAPPER}" "${DEVJOPAK_ECJ_DIR}/bin/java"
        COMMAND chmod +x "${DEVJOPAK_ECJ_DIR}/bin/java"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_ECJ_DIR}/lib/ecj.jar"
        DEPENDS ecj "${DEVJOPAK_ECJ_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${ECJ_JAR}" "${DEVJOPAK_ECJ_DIR}/lib/ecj.jar"
    )
    
    # Copy common libs (jamvm, classes.zip, glibj.zip, native)
    add_custom_command(
        OUTPUT "${DEVJOPAK_ECJ_DIR}/lib/jamvm"
        DEPENDS jamvm_with_gnucp "${DEVJOPAK_ECJ_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${JAMVM_INSTALL_DIR}/bin/jamvm" "${DEVJOPAK_ECJ_DIR}/lib/jamvm"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_ECJ_DIR}/lib/classes.zip"
        DEPENDS jamvm_with_gnucp "${DEVJOPAK_ECJ_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${JAMVM_INSTALL_DIR}/share/jamvm/classes.zip" "${DEVJOPAK_ECJ_DIR}/lib/classes.zip"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_ECJ_DIR}/lib/glibj.zip"
        DEPENDS gnu_classpath "${DEVJOPAK_ECJ_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${CLASSPATH_INSTALL_DIR}/share/classpath/glibj.zip" "${DEVJOPAK_ECJ_DIR}/lib/glibj.zip"
    )

    add_custom_command(
        OUTPUT "${DEVJOPAK_ECJ_DIR}/lib/native/.created"
        DEPENDS gnu_classpath "${DEVJOPAK_ECJ_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEVJOPAK_ECJ_DIR}/lib/native"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CLASSPATH_INSTALL_DIR}/lib/classpath"
            "${DEVJOPAK_ECJ_DIR}/lib/native"
        COMMAND ${CMAKE_COMMAND} -E touch "${DEVJOPAK_ECJ_DIR}/lib/native/.created"
    )
    
    add_custom_command(
        OUTPUT "${DEVJOPAK_ECJ_DIR}/bin/devjopak-validate"
        DEPENDS "${DEVJOPAK_ECJ_DIR}/bin/.created" "${CMAKE_CURRENT_SOURCE_DIR}/devjopak-validate.sh"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/devjopak-validate.sh" "${DEVJOPAK_ECJ_DIR}/bin/devjopak-validate"
        COMMAND chmod +x "${DEVJOPAK_ECJ_DIR}/bin/devjopak-validate"
    )
    
    add_custom_command(
        OUTPUT "${DEVJOPAK_ECJ_DIR}/LICENSE"
        DEPENDS "${DEVJOPAK_ECJ_DIR}/bin/.created"
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/../LICENSE" "${DEVJOPAK_ECJ_DIR}/LICENSE"
    )

    add_custom_command(
        OUTPUT "${CMAKE_BINARY_DIR}/${DEVJOPAK_ECJ_ARCHIVE}"
        DEPENDS
            "${JAVAC_ECJ_WRAPPER}"
            "${DEVJOPAK_ECJ_DIR}/bin/java"
            "${DEVJOPAK_ECJ_DIR}/lib/ecj.jar"
            "${DEVJOPAK_ECJ_DIR}/lib/jamvm"
            "${DEVJOPAK_ECJ_DIR}/lib/classes.zip"
            "${DEVJOPAK_ECJ_DIR}/lib/glibj.zip"
            "${DEVJOPAK_ECJ_DIR}/lib/native/.created"
            "${DEVJOPAK_ECJ_DIR}/bin/devjopak-validate"
            "${DEVJOPAK_ECJ_DIR}/LICENSE"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E tar czf "${DEVJOPAK_ECJ_ARCHIVE}" "devjopak-ecj"
    )

    add_custom_target(devjopak-ecj
        DEPENDS "${CMAKE_BINARY_DIR}/${DEVJOPAK_ECJ_ARCHIVE}"
        COMMENT "DevJopaK-ECJ - Java Development Kit with ECJ compiler"
    )
endif()
