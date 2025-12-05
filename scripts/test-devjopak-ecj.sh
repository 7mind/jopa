#!/usr/bin/env bash
set -e

# project root
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build"
DEVJOPAK_ECJ_BUILD_DIR="$ROOT_DIR/build-devjopak-ecj"
TEST_DIR="$DEVJOPAK_ECJ_BUILD_DIR/test-devjopak-ecj"

# Check if we should use nix build
USE_NIX=false
ECJ_VERSION="4.2.1"

while [ $# -gt 0 ]; do
    case "$1" in
        --nix)
            USE_NIX=true
            shift
            ;;
        --ecj-version)
            ECJ_VERSION="$2"
            shift 2
            ;;
        *)
            shift
            ;;
    esac
done

# Map ECJ version to package name
case "$ECJ_VERSION" in
    4.2.1) NIX_PACKAGE="devjopak-gnucp099-ecj421" ;;
    4.2.2) NIX_PACKAGE="devjopak-gnucp099-ecj422" ;;
    *) echo "Error: Unknown ECJ version $ECJ_VERSION"; exit 1 ;;
esac

if [ "$USE_NIX" = true ]; then
    echo "Building DevJopaK-ECJ (ECJ $ECJ_VERSION) via Nix..."
    cd "$ROOT_DIR"
    nix build ".#$NIX_PACKAGE" --print-build-logs

    DEVJOPAK_HOME="$ROOT_DIR/result"
else
    echo "Building JOPA Compiler..."
    cmake -S "$ROOT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" --target jopa jopa-stub-rt

    echo "Building DevJopaK-ECJ Distribution (ECJ $ECJ_VERSION)..."
    cmake -S "$ROOT_DIR/devjopak" -B "$DEVJOPAK_ECJ_BUILD_DIR" \
        -DJOPA_BUILD_DIR="$BUILD_DIR" \
        -DECJ_VERSION="$ECJ_VERSION" \
        -DCMAKE_BUILD_TYPE=Release

    cmake --build "$DEVJOPAK_ECJ_BUILD_DIR" --target devjopak-ecj

    # Find the archive (handling version variations)
    # New naming: devjopak-{version}-gnucp-{version}-ecj-{version}.tar.gz
    DIST_ARCHIVE=$(find "$DEVJOPAK_ECJ_BUILD_DIR" -name "devjopak-*-ecj-*.tar.gz" | head -n 1)
    if [ -z "$DIST_ARCHIVE" ]; then
        echo "Error: Could not find devjopak-*-ecj-*.tar.gz in $DEVJOPAK_ECJ_BUILD_DIR"
        exit 1
    fi
    echo "Found archive: $DIST_ARCHIVE"

    echo "Setting up test environment in $TEST_DIR..."
    rm -rf "$TEST_DIR"
    mkdir -p "$TEST_DIR"
    tar -xzf "$DIST_ARCHIVE" -C "$TEST_DIR"

    # Adjust path to point to unpacked devjopak-ecj folder
    DEVJOPAK_HOME="$TEST_DIR/devjopak-ecj"
    # Ensure binaries are executable
    chmod -R +x "$DEVJOPAK_HOME/bin"
fi

export JAVA_HOME="$DEVJOPAK_HOME"
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

# Create test directory
TEST_WORK_DIR="${TEST_DIR:-/tmp/test-devjopak-ecj-$$}"
mkdir -p "$TEST_WORK_DIR"

# Create Hello.java
cat > "$TEST_WORK_DIR/Hello.java" <<EOF
public class Hello {
    public static void main(String[] args) {
        System.out.println("Hello from DevJopaK-ECJ!");
    }
}
EOF

cd "$TEST_WORK_DIR"
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
    echo "SUCCESS: DevJopaK-ECJ (ECJ $ECJ_VERSION) passed verification!"
    echo "------------------------------------------------"
else
    echo "------------------------------------------------"
    echo "FAILURE: Unexpected output"
    echo "------------------------------------------------"
    exit 1
fi
