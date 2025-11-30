#!/usr/bin/env bash
set -e

# project root
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
DEVJOPAK_ECJ_BUILD_DIR="$ROOT_DIR/build-devjopak-ecj"
TEST_DIR="$DEVJOPAK_ECJ_BUILD_DIR/test-devjopak-ecj"

echo "Building JOPA Compiler..."
cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build "$BUILD_DIR" --target jopa jopa-stub-rt

echo "Building DevJopaK-ECJ Distribution..."
# Configure devjopak build (reuses devjopak folder for ECJ build too)
cmake -S "$ROOT_DIR/devjopak" -B "$DEVJOPAK_ECJ_BUILD_DIR" \
    -DJOPA_BUILD_DIR="$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$DEVJOPAK_ECJ_BUILD_DIR" --target devjopak-ecj

# Find the archive (handling version variations)
DIST_ARCHIVE=$(find "$DEVJOPAK_ECJ_BUILD_DIR" -name "devjopak-ecj-*.tar.gz" | head -n 1)
if [ -z "$DIST_ARCHIVE" ]; then
    echo "Error: Could not find devjopak-ecj-*.tar.gz in $DEVJOPAK_ECJ_BUILD_DIR"
    exit 1
fi
echo "Found archive: $DIST_ARCHIVE"

echo "Setting up test environment in $TEST_DIR..."
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
tar -xzf "$DIST_ARCHIVE" -C "$TEST_DIR"

# Adjust path to point to unpacked devjopak-ecj folder
DEVJOPAK_HOME="$TEST_DIR/devjopak-ecj"
export JAVA_HOME="$DEVJOPAK_HOME"
# Note: ECJ distribution doesn't include Ant currently, so we skip ANT_HOME export or checks
# If you added Ant to DevJopaK-ECJ, uncomment:
# export ANT_HOME="$DEVJOPAK_HOME"
export PATH="$DEVJOPAK_HOME/bin:$PATH"

echo "------------------------------------------------"
echo "Environment Verification"
echo "------------------------------------------------"
echo "Using: $(which java)"
java -version
echo "Using: $(which javac)"
# Check javac version/help output to confirm it's ECJ
javac -help | head -n 1 || true
echo "------------------------------------------------"

# Create Hello.java
cat > "$TEST_DIR/Hello.java" <<EOF
public class Hello {
    public static void main(String[] args) {
        System.out.println("Hello from DevJopaK-ECJ!");
    }
}
EOF

cd "$TEST_DIR"
echo "Compiling Hello.java with ECJ..."
javac Hello.java

if [ ! -f "Hello.class" ]; then
    echo "Error: Compilation failed, Hello.class not found"
    exit 1
fi

echo "Running Hello..."
OUTPUT=$(java Hello)
echo "Output: $OUTPUT"

if [[ "$OUTPUT" == *"Hello from DevJopaK-ECJ!"* ]]; then
    echo "------------------------------------------------"
    echo "SUCCESS: DevJopaK-ECJ passed verification!"
    echo "------------------------------------------------"
else
    echo "------------------------------------------------"
    echo "FAILURE: Unexpected output"
    echo "------------------------------------------------"
    exit 1
fi
