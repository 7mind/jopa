#!/usr/bin/env bash
set -euo pipefail

# Compliance test script - tests full compilation (not just parsing)
# Usage: test-compliance.sh <jdk7|jdk8> [--build]
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

PREFIX="compliance_${JDK_VERSION}_"
DISPLAY_NAME=$(echo "$JDK_VERSION" | tr '[:lower:]' '[:upper:]')
JDK_NUM="${JDK_VERSION#jdk}"

# Configure paths
if [[ "$JDK_VERSION" == "jdk7" ]]; then
    SUBMODULE_DIR="${PROJECT_DIR}/assets/jdk7u-langtools"
    ASSETS_DIR="${SUBMODULE_DIR}/test"
    WHITELIST_FILE="${PROJECT_DIR}/test/jdk7_compliance_whitelist.txt"
    SOURCE_VERSION="1.7"
    LIB_DIRS="${ASSETS_DIR}/com/sun/javadoc/lib:${ASSETS_DIR}/tools/javac/lib:${ASSETS_DIR}/tools/apt/lib:${ASSETS_DIR}/tools/javac/api/lib"
else
    SUBMODULE_DIR="${PROJECT_DIR}/assets/jdk8u_langtools"
    ASSETS_DIR="${SUBMODULE_DIR}/test"
    WHITELIST_FILE="${PROJECT_DIR}/test/jdk8_compliance_whitelist.txt"
    SOURCE_VERSION="1.8"
    # Test library directories
    LIB_DIRS="${ASSETS_DIR}/com/sun/javadoc/lib:${ASSETS_DIR}/tools/javac/lib:${ASSETS_DIR}/tools/apt/lib"
fi

# Check that the required submodule is initialized
if [[ ! -e "${SUBMODULE_DIR}/.git" ]]; then
    echo "Error: Git submodule not initialized: ${SUBMODULE_DIR}" >&2
    echo "" >&2
    echo "Run: git submodule update --init" >&2
    exit 1
fi

# Check whitelist exists
if [[ ! -f "$WHITELIST_FILE" ]]; then
    echo "Error: Compliance whitelist not found: ${WHITELIST_FILE}" >&2
    echo "" >&2
    echo "Run: python scripts/prepare_jdk_compliance_tests.py --${JDK_VERSION}" >&2
    exit 1
fi

# Build if requested
if [[ "$BUILD_FLAG" == "--build" ]]; then
    echo "=== Configuring Build ==="
    cmake -B "${BUILD_DIR}" -S "${PROJECT_DIR}" \
        -DCMAKE_BUILD_TYPE=Debug \
        -DJOPA_ENABLE_SANITIZERS=ON \
        -DJOPA_ENABLE_CPPTRACE=ON \
        -DJOPA_ENABLE_JDK_COMPLIANCE_TESTS=ON \
        -DJOPA_COMPLIANCE_TESTS_JDK7=$([[ "$JDK_VERSION" == "jdk7" ]] && echo "ON" || echo "OFF") \
        -DJOPA_COMPLIANCE_TESTS_JDK8=$([[ "$JDK_VERSION" == "jdk8" ]] && echo "ON" || echo "OFF")

    echo ""
    echo "=== Building ==="
    cmake --build "${BUILD_DIR}" --parallel "${NPROC}"
fi

# Check build directory and jopa executable exist
JOPA="${BUILD_DIR}/src/jopa"
RUNTIME_JAR="${BUILD_DIR}/runtime/jopa-stub-rt.jar"

if [[ ! -x "$JOPA" ]]; then
    echo "Error: JOPA executable not found. Run with --build or build first." >&2
    exit 1
fi

if [[ ! -f "$RUNTIME_JAR" ]]; then
    echo "Error: Runtime JAR not found. Run with --build or build first." >&2
    exit 1
fi

echo ""
echo "=== Running ${DISPLAY_NAME} Compliance Tests ==="

# Create output directory
TEST_OUTPUT_DIR="${BUILD_DIR}/compliance_${JDK_VERSION}"
mkdir -p "$TEST_OUTPUT_DIR"

# Read whitelist count
WHITELISTED=$(wc -l < "$WHITELIST_FILE" | tr -d ' ')

# Get total .java file count from assets
TOTAL=$(find "$ASSETS_DIR" -name "*.java" 2>/dev/null | wc -l | tr -d ' ')

echo "Total files: ${TOTAL}"
echo "Whitelisted: ${WHITELISTED}"
echo ""

# Run tests in parallel
RESULTS_FILE="${TEST_OUTPUT_DIR}/results.txt"
PASSED_FILE="${TEST_OUTPUT_DIR}/passed.txt"
FAILED_FILE="${TEST_OUTPUT_DIR}/failed.txt"
TIMEOUT_FILE="${TEST_OUTPUT_DIR}/timeout.txt"

: > "$RESULTS_FILE"
: > "$PASSED_FILE"
: > "$FAILED_FILE"
: > "$TIMEOUT_FILE"

# Timeout per test (seconds)
TEST_TIMEOUT=30

echo "Running compilation tests (timeout: ${TEST_TIMEOUT}s per test)..."

# Process tests with xargs for parallelism
cat "$WHITELIST_FILE" | xargs -P "$NPROC" -I {} sh -c '
    rel_path="{}"
    java_file="'"$ASSETS_DIR"'/$rel_path"
    test_out="'"$TEST_OUTPUT_DIR"'/classes/$(dirname "$rel_path")"
    mkdir -p "$test_out" 2>/dev/null || true
    if timeout '"$TEST_TIMEOUT"' "'"$JOPA"'" -source "'"$SOURCE_VERSION"'" -target 1.5 \
        -classpath "'"$RUNTIME_JAR"'" \
        -sourcepath "'"$LIB_DIRS"':$(dirname "$java_file")" \
        -d "$test_out" \
        "$java_file" >/dev/null 2>&1; then
        echo "PASS $rel_path"
    else
        exit_code=$?
        if [ $exit_code -eq 124 ]; then
            echo "TIMEOUT $rel_path"
        else
            echo "FAIL $rel_path"
        fi
    fi
' > "$RESULTS_FILE" 2>&1 || true

# Count results (ensure clean numeric values)
PASSED=$(grep -c "^PASS " "$RESULTS_FILE" 2>/dev/null || echo 0)
PASSED=$(echo "$PASSED" | tr -d '\n\r ')
FAILED=$(grep -c "^FAIL " "$RESULTS_FILE" 2>/dev/null || echo 0)
FAILED=$(echo "$FAILED" | tr -d '\n\r ')
TIMEOUT=$(grep -c "^TIMEOUT " "$RESULTS_FILE" 2>/dev/null || echo 0)
TIMEOUT=$(echo "$TIMEOUT" | tr -d '\n\r ')

# Extract lists
grep "^PASS " "$RESULTS_FILE" | sed 's/^PASS //' > "$PASSED_FILE" 2>/dev/null || true
grep "^FAIL " "$RESULTS_FILE" | sed 's/^FAIL //' > "$FAILED_FILE" 2>/dev/null || true
grep "^TIMEOUT " "$RESULTS_FILE" | sed 's/^TIMEOUT //' > "$TIMEOUT_FILE" 2>/dev/null || true

# Calculate pass rate
if [[ "$WHITELISTED" -gt 0 ]]; then
    PASS_RATE=$(awk "BEGIN {printf \"%.1f\", ($PASSED / $WHITELISTED) * 100}")
else
    PASS_RATE="0.0"
fi

# Console output
echo ""
echo "${DISPLAY_NAME} Compliance Tests:"
echo "  Total:       ${TOTAL}"
echo "  Whitelisted: ${WHITELISTED}"
echo "  Passed:      ${PASSED} (${PASS_RATE}%)"
echo "  Failed:      ${FAILED}"
echo "  Timeout:     ${TIMEOUT}"

# Write summary (markdown format for GitHub, plain for stdout)
{
    echo "## ${DISPLAY_NAME} Compliance Test Results"
    echo ""
    echo "| Metric | Count |"
    echo "|--------|-------|"
    echo "| Total Files | ${TOTAL} |"
    echo "| Whitelisted | ${WHITELISTED} |"
    echo "| Passed | ${PASSED} (${PASS_RATE}%) |"
    echo "| Failed | ${FAILED} |"
    echo "| Timeout | ${TIMEOUT} |"
    echo ""
    if [[ "$FAILED" -gt 0 ]]; then
        echo "### Sample Failed Tests (first 20)"
        echo '```'
        head -20 "$FAILED_FILE" 2>/dev/null || echo 'No failures'
        echo '```'
    fi
    if [[ "$TIMEOUT" -gt 0 ]]; then
        echo "### Timeout Tests"
        echo '```'
        cat "$TIMEOUT_FILE" 2>/dev/null || echo 'No timeouts'
        echo '```'
    fi
} >> "$OUTPUT"

# Write results file for parent script
RESULTS_ENV_FILE="${BUILD_DIR}/compliance_${JDK_VERSION}_results.env"
{
    echo "COMPLIANCE_TOTAL=$TOTAL"
    echo "COMPLIANCE_WHITELISTED=$WHITELISTED"
    echo "COMPLIANCE_PASSED=$PASSED"
    echo "COMPLIANCE_FAILED=$FAILED"
    echo "COMPLIANCE_TIMEOUT=$TIMEOUT"
    echo "COMPLIANCE_PASS_RATE=$PASS_RATE"
} > "$RESULTS_ENV_FILE"

echo ""
echo "Results saved to: ${TEST_OUTPUT_DIR}"
echo "  Passed: ${PASSED_FILE}"
echo "  Failed: ${FAILED_FILE}"
echo "  Timeout: ${TIMEOUT_FILE}"

# Exit with error if pass rate is below threshold (default 40%)
THRESHOLD="${COMPLIANCE_THRESHOLD:-40}"
if awk "BEGIN {exit !($PASS_RATE < $THRESHOLD)}"; then
    echo ""
    echo "Warning: Pass rate ${PASS_RATE}% is below threshold ${THRESHOLD}%"
fi

exit 0
