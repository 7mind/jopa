#!/usr/bin/env bash
set -e

# project root
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
DEVJOPAK_BUILD_DIR="$ROOT_DIR/build-devjopak"
TEST_DIR="$DEVJOPAK_BUILD_DIR/test-devjopak"

# Clean up previous builds to ensure fresh state
# rm -rf "$DEVJOPAK_BUILD_DIR"

echo "Building JOPA Compiler..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target jopa jopa-stub-rt

echo "Building DevJopaK Distribution..."
# Configure devjopak build, pointing to the jopa build artifacts
cmake -S "$ROOT_DIR/devjopak" -B "$DEVJOPAK_BUILD_DIR" \
    -DJOPA_BUILD_DIR="$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$DEVJOPAK_BUILD_DIR" --target devjopak

# Find the archive (handling version variations)
DIST_ARCHIVE=$(find "$DEVJOPAK_BUILD_DIR" -name "devjopak-*.tar.gz" | head -n 1)
if [ -z "$DIST_ARCHIVE" ]; then
    echo "Error: Could not find devjopak-*.tar.gz in $DEVJOPAK_BUILD_DIR"
    exit 1
fi
echo "Found archive: $DIST_ARCHIVE"

echo "Setting up test environment in $TEST_DIR..."
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
tar -xzf "$DIST_ARCHIVE" -C "$TEST_DIR"

DEVJOPAK_HOME="$TEST_DIR/devjopak"
export JAVA_HOME="$DEVJOPAK_HOME"
export ANT_HOME="$DEVJOPAK_HOME"
export PATH="$DEVJOPAK_HOME/bin:$PATH"

echo "------------------------------------------------"
echo "Environment Verification"
echo "------------------------------------------------"
echo "Using: $(which java)"
java -version
echo "Using: $(which javac)"
javac -version
echo "Using: $(which ant)"
ant -version
echo "------------------------------------------------"

# Create Hello.java
cat > "$TEST_DIR/Hello.java" <<EOF
public class Hello {
    public static void main(String[] args) {
        System.out.println("Hello from DevJopaK!");
    }
    
    public String getMessage() {
        return "Hello";
    }
}
EOF

# Create HelloTest.java (JUnit 3)
cat > "$TEST_DIR/HelloTest.java" <<EOF
import junit.framework.TestCase;

public class HelloTest extends TestCase {
    public void testMessage() {
        Hello h = new Hello();
        assertEquals("Hello", h.getMessage());
    }
}
EOF

# Create build.xml
cat > "$TEST_DIR/build.xml" <<EOF
<project name="TestDevJopaK" default="test" basedir=".">
    <target name="init">
        <mkdir dir="classes"/>
        <mkdir dir="test-classes"/>
        <mkdir dir="reports"/>
    </target>

    <target name="compile" depends="init">
        <javac srcdir="." destdir="classes" includeantruntime="false" source="1.6" target="1.6" fork="yes" executable="javac">
            <include name="Hello.java"/>
        </javac>
    </target>

    <target name="compile-tests" depends="compile">
        <javac srcdir="." destdir="test-classes" includeantruntime="false" source="1.6" target="1.6" fork="yes" executable="javac">
            <include name="HelloTest.java"/>
            <classpath>
                <pathelement path="classes"/>
                <fileset dir="\${ant.home}/lib">
                    <include name="junit.jar"/>
                </fileset>
            </classpath>
        </javac>
    </target>

    <target name="run" depends="compile">
        <java classname="Hello" classpath="classes" failonerror="true" fork="yes" jvm="java"/>
    </target>

    <target name="test" depends="compile-tests">
        <junit printsummary="yes" haltonfailure="true" fork="yes" jvm="java">
            <classpath>
                <pathelement path="classes"/>
                <pathelement path="test-classes"/>
                <fileset dir="\${ant.home}/lib">
                    <include name="junit.jar"/>
                </fileset>
            </classpath>
            <formatter type="plain"/>
            <test name="HelloTest" todir="reports"/>
        </junit>
    </target>
</project>
EOF

cd "$TEST_DIR"
echo "Running Ant run..."
ant run

echo "Running Ant test..."
ant test

echo "------------------------------------------------"
echo "SUCCESS: DevJopaK passed verification!"
echo "------------------------------------------------"