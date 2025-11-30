# ============================================================================
# Eclipse Compiler for Java (ECJ) Build
# ============================================================================
if(NOT JOPA_CLASSPATH_VERSION STREQUAL "0.93")
    set(ECJ_VERSION "4.2.1")
    set(ECJ_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../vendor/ecj-${ECJ_VERSION}-src")
    set(ECJ_BUILD_DIR "${CMAKE_BINARY_DIR}/ecj-build")
    set(ECJ_INSTALL_DIR "${VENDOR_PREFIX}/ecj")
    set(ECJ_JAR "${ECJ_INSTALL_DIR}/lib/ecj.jar")

    add_custom_command(
        OUTPUT "${ECJ_JAR}"
        DEPENDS jamvm_with_gnucp
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ECJ_BUILD_DIR}/classes"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${ECJ_INSTALL_DIR}/lib"
        
        # 1. Copy sources to a temporary build src directory
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${ECJ_BUILD_DIR}/src"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ECJ_SOURCE_DIR}" "${ECJ_BUILD_DIR}/src"
        
        # 2. Remove problematic sources that depend on missing APIs (javax.tools, javax.annotation.processing)
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${ECJ_BUILD_DIR}/src/org/eclipse/jdt/internal/compiler/tool"
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${ECJ_BUILD_DIR}/src/org/eclipse/jdt/internal/compiler/apt"
        COMMAND ${CMAKE_COMMAND} -E remove "${ECJ_BUILD_DIR}/src/org/eclipse/jdt/core/JDTCompilerAdapter.java"
        
        # 3. Compile the remaining sources
        # We use 'find' to generate the list of files to compile from the pruned source tree
        COMMAND sh -c "find \"${ECJ_BUILD_DIR}/src\" -name '*.java' > \"${ECJ_BUILD_DIR}/sources.list\""
        
        COMMAND ${CMAKE_COMMAND} -E env
            ${CLEAN_JAVA_ENV_VARS}
            "${CMAKE_CURRENT_BINARY_DIR}/ant-javac.sh"
            -d "${ECJ_BUILD_DIR}/classes"
            -source 1.5 -target 1.5
            -encoding ISO-8859-1
            -nowarn
            "@${ECJ_BUILD_DIR}/sources.list"
            
        # 4. Copy resources (properties, etc.) to classes dir
        # We reuse the source tree for this, identifying non-java files
        COMMAND sh -c "find \"${ECJ_BUILD_DIR}/src\" -type f -not -name '*.java' -exec cp --parents -t \"${ECJ_BUILD_DIR}/classes\" {} + || true"
        # The cp --parents might fail or behave weirdly depending on CWD. 
        # Safer approach: Copy everything again to classes, then delete .java
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ECJ_BUILD_DIR}/src" "${ECJ_BUILD_DIR}/classes"
        COMMAND find "${ECJ_BUILD_DIR}/classes" -name "*.java" -delete

        # 5. Package into JAR
        COMMAND ${CMAKE_COMMAND} -E chdir "${ECJ_BUILD_DIR}/classes" "${ZIP_WRAPPER}" -r "${ECJ_JAR}" .
        
        COMMENT "Building Eclipse Compiler for Java (ECJ) ${ECJ_VERSION}"
    )

    add_custom_target(ecj DEPENDS "${ECJ_JAR}")

    # ============================================================================ 
    # DevJopaK-ECJ Distribution
    # ============================================================================ 
    set(DEVJOPAK_ECJ_DIR "${CMAKE_BINARY_DIR}/devjopak-ecj")
    set(DEVJOPAK_ECJ_ARCHIVE "devjopak-ecj-${JOPA_VERSION}.tar.gz")
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
