#!/usr/bin/env bash
set -euo pipefail

# Primary test suite - runs semantic/bytecode tests with target version matrix
# All tests use -source 1.7, targets: 1.5, 1.6, 1.7
#
# Usage: test-primary.sh [--quick]
#   --quick    Only run with default target (1.5), skip matrix

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Check for --quick flag
QUICK_MODE=false
if [[ "${1:-}" == "--quick" ]]; then
    QUICK_MODE=true
fi

echo "=== Jopa Primary Test Suite ==="
echo "Project: ${PROJECT_DIR}"
echo "Build:   ${BUILD_DIR}"
echo "Parallel jobs: ${NPROC}"
echo ""

# Target versions to test
if $QUICK_MODE; then
    TARGETS=("1.5")
    echo "Mode: Quick (target 1.5 only)"
else
    TARGETS=("1.5" "1.6" "1.7")
    echo "Mode: Full matrix (targets 1.5, 1.6, 1.7)"
fi
echo ""

FAILED=false
RESULTS=()

for TARGET in "${TARGETS[@]}"; do
    echo "========================================"
    echo "  Testing with -source 1.7 -target ${TARGET}"
    echo "========================================"
    echo ""

    # Clean test output directories
    rm -rf "${BUILD_DIR}/test/"*/

    # Configure
    echo "Configuring..."
    cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" \
        -DJOPA_TARGET_VERSION="${TARGET}" \
        -DJOPA_ENABLE_SANITIZERS=ON \
        -DCMAKE_BUILD_TYPE=Debug \
        -DJOPA_ENABLE_JDK_PARSER_TESTS=OFF \
        >/dev/null 2>&1

    # Build
    echo "Building..."
    cmake --build "${BUILD_DIR}" --parallel "${NPROC}" >/dev/null 2>&1

    # Run tests
    echo "Running tests..."
    cd "${BUILD_DIR}"

    # Count tests
    TEST_COUNT=$(ctest -N 2>/dev/null | grep "Total Tests:" | sed 's/Total Tests: //' | tr -d ' ')

    # Run tests
    set +e
    OUTPUT=$(ctest --output-on-failure -j"${NPROC}" 2>&1)
    EXIT_CODE=$?
    set -e

    # Extract pass/fail counts
    SUMMARY=$(echo "$OUTPUT" | grep -E "tests passed|tests failed" | tail -1)

    if [ $EXIT_CODE -eq 0 ]; then
        RESULTS+=("Target ${TARGET}: PASS - ${SUMMARY}")
        echo "✓ Target ${TARGET}: ALL ${TEST_COUNT} TESTS PASSED"
    else
        RESULTS+=("Target ${TARGET}: FAIL - ${SUMMARY}")
        echo "✗ Target ${TARGET}: FAILURES"
        echo "$OUTPUT" | tail -30
        FAILED=true
    fi
    echo ""
done

echo "========================================"
echo "           SUMMARY"
echo "========================================"
echo ""
for RESULT in "${RESULTS[@]}"; do
    echo "  $RESULT"
done
echo ""

if $FAILED; then
    echo "OVERALL: SOME TESTS FAILED"
    exit 1
else
    echo "OVERALL: ALL TESTS PASSED"
    exit 0
fi
