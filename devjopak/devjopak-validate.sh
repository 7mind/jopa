#!/usr/bin/env bash
#
# This script validates a DevJopaK distribution by compiling and running a simple Java program.
#

set -euo pipefail

# Determine the directory where this script resides.
# This implies DEVJOPAK_HOME is the parent directory of 'bin'.
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEVJOPAK_HOME="$(dirname "$SCRIPT_DIR")"

echo "=== Verifying DevJopaK Installation ==="
echo "DEVJOPAK_HOME: $DEVJOPAK_HOME"
echo ""

# Export environment variables for verification
export JAVA_HOME="$DEVJOPAK_HOME"
export ANT_HOME="$DEVJOPAK_HOME" # Not strictly needed for this script, but good for consistency
export PATH="$DEVJOPAK_HOME/bin:$PATH"

echo "--- Version Information ---"
echo "java -version:"
java -version
echo "javac -version:"
javac -version
echo ""

echo "--- Compiling and Running HelloWorld ---"
TEST_DIR="$(mktemp -d)"
echo "Working directory: $TEST_DIR"
cd "$TEST_DIR"

cat > HelloWorld.java <<EOF
public class HelloWorld {
    public static void main(String[] args) {
        System.out.println("Hello, DevJopaK!");
    }
}
EOF

echo "Compiling HelloWorld.java..."
javac HelloWorld.java

echo "Running HelloWorld..."
java HelloWorld

if [ -f HelloWorld.class ]; then
    echo "SUCCESS: HelloWorld.class created."
else
    echo "ERROR: HelloWorld.class not found after compilation."
    rm -rf "$TEST_DIR"
    exit 1
fi

if grep -q "Hello, DevJopaK!" <<< "$(java HelloWorld)"; then
    echo "SUCCESS: Program executed and output verified."
else
    echo "ERROR: Program output 'Hello, DevJopaK!' not found."
    rm -rf "$TEST_DIR"
    exit 1
fi

echo ""
echo "--- Cleanup ---"
rm -rf "$TEST_DIR"
echo "Temporary files removed from $TEST_DIR"

echo ""
echo "=== DevJopaK Validation SUCCESS ==="
exit 0
