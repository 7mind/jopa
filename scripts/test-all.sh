#!/usr/bin/env bash
set -euo pipefail

# Complete test suite - runs primary tests and parser tests
# Usage: test-all.sh
#
# If GITHUB_STEP_SUMMARY is set, outputs markdown to it
# Otherwise outputs to stdout

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Output destination (GITHUB_STEP_SUMMARY or stdout)
OUTPUT="${GITHUB_STEP_SUMMARY:-/dev/stdout}"

echo "=== Jopa Compiler Test Suite ==="
echo "Project: ${PROJECT_DIR}"
echo "Build:   ${BUILD_DIR}"
echo "Parallel jobs: ${NPROC}"
echo ""

# Check that submodules are initialized
JDK7_ASSETS="${PROJECT_DIR}/assets/jdk7u-langtools"
JDK8_ASSETS="${PROJECT_DIR}/assets/jdk8u_langtools"

if [[ ! -e "${JDK7_ASSETS}/.git" ]] || [[ ! -e "${JDK8_ASSETS}/.git" ]]; then
    echo "Error: Git submodules are not initialized." >&2
    echo "" >&2
    echo "Run: git submodule update --init" >&2
    echo "" >&2
    [[ ! -e "${JDK7_ASSETS}/.git" ]] && echo "  Missing: assets/jdk7u-langtools" >&2
    [[ ! -e "${JDK8_ASSETS}/.git" ]] && echo "  Missing: assets/jdk8u_langtools" >&2
    exit 1
fi

# Configure and build (Debug with sanitizers for primary tests)
echo "=== Configuring Build ==="
CMAKE_OUTPUT=$(cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" \
    -DJOPA_ENABLE_SANITIZERS=ON \
    -DJOPA_ENABLE_CPPTRACE=ON \
    -DJOPA_ENABLE_JDK_PARSER_TESTS=ON \
    -DCMAKE_BUILD_TYPE=Debug 2>&1)
echo "$CMAKE_OUTPUT"

# Extract and save skipped counts for parser tests
JDK7_SKIPPED_COUNT=$(echo "$CMAKE_OUTPUT" | grep -oP "Added \d+ JDK7 parser tests \(skipped \K\d+" || echo "0")
JDK8_SKIPPED_COUNT=$(echo "$CMAKE_OUTPUT" | grep -oP "Added \d+ JDK8 parser tests \(skipped \K\d+" || echo "0")
echo "$JDK7_SKIPPED_COUNT" > "${BUILD_DIR}/parser_jdk7_skipped.txt"
echo "$JDK8_SKIPPED_COUNT" > "${BUILD_DIR}/parser_jdk8_skipped.txt"

echo ""
echo "=== Building Compiler and Runtime ==="
cmake --build "${BUILD_DIR}" --parallel "${NPROC}"

echo ""
echo "=== Running Primary Test Suite ==="
cd "${BUILD_DIR}"

# Get primary test count
PRIMARY_TOTAL=$(ctest -N -E "^parse_" 2>/dev/null | grep "Total Tests:" | sed 's/Total Tests: //' | tr -d ' ')
PRIMARY_TOTAL=${PRIMARY_TOTAL:-0}

# Run primary tests (exclude parser tests) in parallel
set +e
PRIMARY_OUTPUT=$(ctest --output-on-failure -j"${NPROC}" -E "^parse_" 2>&1)
PRIMARY_EXIT=$?
set -e

echo "$PRIMARY_OUTPUT"

if [ $PRIMARY_EXIT -eq 0 ]; then
    echo ""
    echo "PRIMARY TESTS: ALL ${PRIMARY_TOTAL} PASSED"
else
    echo ""
    echo "PRIMARY TESTS: SOME FAILURES (see above)"
fi

# Write primary test summary
{
    echo "## Primary Test Results"
    echo ""
    echo '```'
    echo "$PRIMARY_OUTPUT" | tail -20
    echo '```'
} >> "$OUTPUT"

# Run parser tests using the dedicated scripts
echo ""
echo "=== Running JDK 7/8 Parser Tests (in parallel) ==="

# JDK7 Parser Tests
echo ""
echo "--- JDK7 Parser Tests ---"
"${SCRIPT_DIR}/test-parser.sh" jdk7
# shellcheck source=/dev/null
source "${BUILD_DIR}/parser_jdk7_results.env"
JDK7_TOTAL=${PARSER_TOTAL:-0}
JDK7_SKIPPED=${PARSER_SKIPPED:-0}
JDK7_PASSED=${PARSER_PASSED:-0}
JDK7_FAILED=${PARSER_FAILED:-0}
JDK7_TIMEOUT=${PARSER_TIMEOUT:-0}
JDK7_CRASHED=${PARSER_CRASHED:-0}

# JDK8 Parser Tests
echo ""
echo "--- JDK8 Parser Tests ---"
"${SCRIPT_DIR}/test-parser.sh" jdk8
# shellcheck source=/dev/null
source "${BUILD_DIR}/parser_jdk8_results.env"
JDK8_TOTAL=${PARSER_TOTAL:-0}
JDK8_SKIPPED=${PARSER_SKIPPED:-0}
JDK8_PASSED=${PARSER_PASSED:-0}
JDK8_FAILED=${PARSER_FAILED:-0}
JDK8_TIMEOUT=${PARSER_TIMEOUT:-0}
JDK8_CRASHED=${PARSER_CRASHED:-0}

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
echo "  Total:   ${PRIMARY_TOTAL}"
echo ""
echo "JDK7 Parser Tests:"
echo "  Total:   ${JDK7_TOTAL}"
echo "  Skipped: ${JDK7_SKIPPED} (negative tests)"
echo "  Passed:  ${JDK7_PASSED}"
echo "  Failed:  ${JDK7_FAILED}"
echo "  Timeout: ${JDK7_TIMEOUT}"
echo "  Crashed: ${JDK7_CRASHED}"
echo ""
echo "JDK8 Parser Tests:"
echo "  Total:   ${JDK8_TOTAL}"
echo "  Skipped: ${JDK8_SKIPPED} (negative tests)"
echo "  Passed:  ${JDK8_PASSED}"
echo "  Failed:  ${JDK8_FAILED}"
echo "  Timeout: ${JDK8_TIMEOUT}"
echo "  Crashed: ${JDK8_CRASHED}"
echo ""
echo "========================================"

# Exit with primary test status (parser test failures are expected)
exit $PRIMARY_EXIT
