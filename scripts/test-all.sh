#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"

echo "=== Jopa Compiler Test Suite ==="
echo "Project: ${PROJECT_DIR}"
echo "Build:   ${BUILD_DIR}"
echo ""

# Configure and build
echo "=== Configuring Build ==="
cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" \
    -DJOPA_ENABLE_SANITIZERS=ON \
    -DJOPA_ENABLE_CPPTRACE=ON \
    -DJOPA_ENABLE_JDK_PARSER_TESTS=ON \
    -DCMAKE_BUILD_TYPE=Debug

echo ""
echo "=== Building Compiler and Runtime ==="
cmake --build "${BUILD_DIR}" --parallel

echo ""
echo "=== Running Primary Test Suite ==="
cd "${BUILD_DIR}"

# Run primary tests (exclude parser tests)
set +e
ctest --output-on-failure -E "^parse_" 2>&1
PRIMARY_EXIT=$?
set -e

# Get primary test counts
PRIMARY_RESULTS=$(ctest -N -E "^parse_" | tail -1)
PRIMARY_TOTAL=$(echo "$PRIMARY_RESULTS" | grep -oP '\d+(?= tests)')

if [ $PRIMARY_EXIT -eq 0 ]; then
    echo ""
    echo "PRIMARY TESTS: ALL ${PRIMARY_TOTAL} PASSED"
else
    echo ""
    echo "PRIMARY TESTS: SOME FAILURES (see above)"
fi

echo ""
echo "=== Running JDK 7/8 Parser Tests ==="

# Run JDK7 parser tests
echo ""
echo "--- JDK7 Parser Tests ---"
set +e
JDK7_OUTPUT=$(ctest --output-on-failure -R "^parse_jdk7_" 2>&1)
JDK7_EXIT=$?
set -e

JDK7_TOTAL=$(echo "$JDK7_OUTPUT" | grep -oP '\d+(?= tests)' | head -1 || echo "0")
JDK7_PASSED=$(echo "$JDK7_OUTPUT" | grep -oP '\d+(?= tests passed)' || echo "0")
JDK7_FAILED=$(echo "$JDK7_OUTPUT" | grep -oP '\d+(?= tests failed)' || echo "0")

# Run JDK8 parser tests
echo ""
echo "--- JDK8 Parser Tests ---"
set +e
JDK8_OUTPUT=$(ctest --output-on-failure -R "^parse_jdk8_" 2>&1)
JDK8_EXIT=$?
set -e

JDK8_TOTAL=$(echo "$JDK8_OUTPUT" | grep -oP '\d+(?= tests)' | head -1 || echo "0")
JDK8_PASSED=$(echo "$JDK8_OUTPUT" | grep -oP '\d+(?= tests passed)' || echo "0")
JDK8_FAILED=$(echo "$JDK8_OUTPUT" | grep -oP '\d+(?= tests failed)' || echo "0")

echo ""
echo "========================================"
echo "           TEST SUMMARY"
echo "========================================"
echo ""
echo "Primary Tests:"
if [ $PRIMARY_EXIT -eq 0 ]; then
    echo "  Status:  PASSED"
else
    echo "  Status:  FAILED"
fi
echo "  Total:   ${PRIMARY_TOTAL:-unknown}"
echo ""
echo "JDK7 Parser Tests:"
if [ $JDK7_EXIT -eq 0 ]; then
    echo "  Status:  PASSED"
else
    echo "  Status:  SOME FAILURES"
fi
echo "  Passed:  ${JDK7_PASSED}"
echo "  Failed:  ${JDK7_FAILED}"
echo ""
echo "JDK8 Parser Tests:"
if [ $JDK8_EXIT -eq 0 ]; then
    echo "  Status:  PASSED"
else
    echo "  Status:  SOME FAILURES"
fi
echo "  Passed:  ${JDK8_PASSED}"
echo "  Failed:  ${JDK8_FAILED}"
echo ""
echo "========================================"

# Exit with primary test status (parser test failures are expected)
exit $PRIMARY_EXIT
