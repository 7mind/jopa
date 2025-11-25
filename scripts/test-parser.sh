#!/usr/bin/env bash
set -euo pipefail

# Parser test script - can be run standalone or from test-all.sh
# Usage: test-parser.sh <jdk7|jdk8> [--build]
#
# If GITHUB_STEP_SUMMARY is set, outputs markdown to it
# Otherwise outputs to stdout

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_DIR}/build"
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Output destination (GITHUB_STEP_SUMMARY or stdout)
OUTPUT="${GITHUB_STEP_SUMMARY:-/dev/stdout}"

# Parse arguments
JDK_VERSION="${1:-}"
BUILD_FLAG="${2:-}"

if [[ -z "$JDK_VERSION" || ! "$JDK_VERSION" =~ ^(jdk7|jdk8)$ ]]; then
    echo "Usage: $0 <jdk7|jdk8> [--build]" >&2
    exit 1
fi

PREFIX="parse_${JDK_VERSION}_"
DISPLAY_NAME=$(echo "$JDK_VERSION" | tr '[:lower:]' '[:upper:]')

# Build if requested
if [[ "$BUILD_FLAG" == "--build" ]]; then
    echo "=== Configuring Build ==="
    CMAKE_OUTPUT=$(cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DJOPA_ENABLE_SANITIZERS=ON \
        -DJOPA_ENABLE_CPPTRACE=ON \
        -DJOPA_ENABLE_JDK_PARSER_TESTS=ON \
        -DJOPA_PARSER_TESTS_JDK7=$([[ "$JDK_VERSION" == "jdk7" ]] && echo "ON" || echo "OFF") \
        -DJOPA_PARSER_TESTS_JDK8=$([[ "$JDK_VERSION" == "jdk8" ]] && echo "ON" || echo "OFF") 2>&1)
    echo "$CMAKE_OUTPUT"

    # Extract skipped count from cmake output and save it
    JDK_NUM="${JDK_VERSION#jdk}"
    SKIPPED=$(echo "$CMAKE_OUTPUT" | grep -oP "Added \d+ JDK${JDK_NUM} parser tests \(skipped \K\d+" || echo "0")
    echo "$SKIPPED" > "${BUILD_DIR}/parser_${JDK_VERSION}_skipped.txt"

    echo ""
    echo "=== Building ==="
    cmake --build "${BUILD_DIR}" --parallel "${NPROC}"
fi

# Check build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Error: Build directory not found. Run with --build or build first." >&2
    exit 1
fi

cd "${BUILD_DIR}"

echo ""
echo "=== Running ${DISPLAY_NAME} Parser Tests ==="

# Run tests and capture output
set +e
TEST_OUTPUT=$(ctest -R "^${PREFIX}" -j"${NPROC}" --output-on-failure 2>&1)
TEST_EXIT=$?
set -e

# Save output for later analysis
echo "$TEST_OUTPUT" > parser_results.txt

# Get total count
TOTAL=$(ctest -R "^${PREFIX}" -N 2>/dev/null | grep "Total Tests:" | sed 's/Total Tests: //' | tr -d ' ')
TOTAL=${TOTAL:-0}

# Get skipped count from saved file (if exists)
SKIPPED_FILE="${BUILD_DIR}/parser_${JDK_VERSION}_skipped.txt"
if [[ -f "$SKIPPED_FILE" ]]; then
    SKIPPED=$(cat "$SKIPPED_FILE" | tr -d ' \n')
else
    SKIPPED=0
fi
SKIPPED=${SKIPPED:-0}

# Count different failure types from the "The following tests FAILED:" section
# Format: "123 - test_name (Failed)" or "(Subprocess aborted)" or "(Timeout)"
FAILED=$(echo "$TEST_OUTPUT" | grep -cE '^\s+[0-9]+.*\(Failed\)') || true
TIMEOUT=$(echo "$TEST_OUTPUT" | grep -cE '^\s+[0-9]+.*\(Timeout\)') || true
CRASHED=$(echo "$TEST_OUTPUT" | grep -cE '^\s+[0-9]+.*\(Subprocess aborted\)') || true

# Ensure numeric values
FAILED=$(echo "$FAILED" | tr -d ' \n')
TIMEOUT=$(echo "$TIMEOUT" | tr -d ' \n')
CRASHED=$(echo "$CRASHED" | tr -d ' \n')
FAILED=${FAILED:-0}
TIMEOUT=${TIMEOUT:-0}
CRASHED=${CRASHED:-0}

TOTAL_FAILURES=$((FAILED + TIMEOUT + CRASHED))
PASSED=$((TOTAL - TOTAL_FAILURES))

# Console output
echo ""
echo "${DISPLAY_NAME} Parser Tests:"
echo "  Total:   ${TOTAL}"
echo "  Skipped: ${SKIPPED} (negative tests)"
echo "  Passed:  ${PASSED}"
echo "  Failed:  ${FAILED}"
echo "  Timeout: ${TIMEOUT}"
echo "  Crashed: ${CRASHED}"

# Write summary (markdown format for GitHub, plain for stdout)
{
    echo "## ${DISPLAY_NAME} Parser Test Results"
    echo ""
    echo "| Metric | Count |"
    echo "|--------|-------|"
    echo "| Total Tests | ${TOTAL} |"
    echo "| Skipped (negative) | ${SKIPPED} |"
    echo "| Passed | ${PASSED} |"
    echo "| Failed | ${FAILED} |"
    echo "| Timeout | ${TIMEOUT} |"
    echo "| Crashed | ${CRASHED} |"
    echo ""
    echo "### Last 30 lines of output"
    echo '```'
    tail -30 parser_results.txt 2>/dev/null || echo 'No results available'
    echo '```'
} >> "$OUTPUT"

# Write results file for parent script
RESULTS_FILE="${BUILD_DIR}/parser_${JDK_VERSION}_results.env"
{
    echo "PARSER_TOTAL=$TOTAL"
    echo "PARSER_SKIPPED=$SKIPPED"
    echo "PARSER_PASSED=$PASSED"
    echo "PARSER_FAILED=$FAILED"
    echo "PARSER_TIMEOUT=$TIMEOUT"
    echo "PARSER_CRASHED=$CRASHED"
} > "$RESULTS_FILE"

# Return success even if tests fail (parser tests are expected to have some failures)
exit 0
