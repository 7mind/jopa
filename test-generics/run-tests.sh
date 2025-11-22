#!/usr/bin/env bash
# Comprehensive test script for Jikes Java 5 features
# Tests both compilation AND runtime execution with real JVM

set -e  # Exit on error unless explicitly handled

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JIKES="${SCRIPT_DIR}/../src/jikes"
RUNTIME="${SCRIPT_DIR}/runtime"
OUTPUT="${SCRIPT_DIR}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

echo "================================================"
echo "  Jikes Java 5 Feature Test Suite"
echo "  Compilation + Runtime Execution Tests"
echo "================================================"
echo ""

# Check if jikes exists
if [ ! -f "$JIKES" ]; then
    echo -e "${RED}ERROR: Jikes compiler not found at $JIKES${NC}"
    echo "Please build it first: make -C ../src"
    exit 1
fi

echo "Using Jikes: $JIKES"
echo "Using Java:  $(java -version 2>&1 | head -1)"
echo ""

# Function to compile a test file
compile_test() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .java)

    echo -n "  Compiling $test_name... "
    if "$JIKES" -source 1.5 -sourcepath "$RUNTIME" -d "$OUTPUT" "$test_file" 2>&1 > /tmp/jikes_compile_$$.log; then
        echo -e "${GREEN}✓${NC}"
        return 0
    else
        echo -e "${RED}✗${NC}"
        echo -e "${RED}    Compilation failed:${NC}"
        cat /tmp/jikes_compile_$$.log | sed 's/^/    /'
        return 1
    fi
}

# Function to run a test class
run_test() {
    local test_class="$1"
    local test_name="$test_class"

    echo -n "  Running $test_name... "

    if java -cp "$OUTPUT" "$test_class" 2>&1 > /tmp/jikes_run_$$.log; then
        echo -e "${GREEN}✓${NC}"
        # Show output if any
        if [ -s /tmp/jikes_run_$$.log ]; then
            cat /tmp/jikes_run_$$.log | sed 's/^/    Output: /'
        fi
        return 0
    else
        echo -e "${RED}✗${NC}"
        echo -e "${RED}    Runtime error:${NC}"
        cat /tmp/jikes_run_$$.log | sed 's/^/    /'
        return 1
    fi
}

# Function to test compilation only (no main method)
test_compile_only() {
    local test_file="$1"
    local test_name=$(basename "$test_file" .java)

    echo -e "${CYAN}Testing: $test_name (compile only)${NC}"
    TESTS_RUN=$((TESTS_RUN + 1))

    if compile_test "$test_file"; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo ""
        return 0
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo ""
        return 1
    fi
}

# Function to test compilation and execution
test_compile_and_run() {
    local test_file="$1"
    local test_class="$2"
    local test_name=$(basename "$test_file" .java)

    echo -e "${CYAN}Testing: $test_name (compile + run)${NC}"
    TESTS_RUN=$((TESTS_RUN + 1))

    if compile_test "$test_file"; then
        if run_test "$test_class"; then
            TESTS_PASSED=$((TESTS_PASSED + 1))
            echo ""
            return 0
        else
            TESTS_FAILED=$((TESTS_FAILED + 1))
            echo ""
            return 1
        fi
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo ""
        return 1
    fi
}

# Clean previous builds
echo "Cleaning previous test outputs..."
rm -f "${OUTPUT}"/*.class /tmp/jikes_*_$$.log
echo ""

# Run tests
echo -e "${BLUE}=== Testing Enums ===${NC}"
test_compile_and_run "${SCRIPT_DIR}/ColorTest.java" "ColorTest" || true
test_compile_only "${SCRIPT_DIR}/Color.java" || true

echo -e "${BLUE}=== Testing Varargs ===${NC}"
test_compile_and_run "${SCRIPT_DIR}/VarargsTest.java" "VarargsTest" || true
test_compile_only "${SCRIPT_DIR}/VarargsImplicitTest.java" || true

echo -e "${BLUE}=== Testing Enhanced For-Loops ===${NC}"
test_compile_and_run "${SCRIPT_DIR}/ForEachTest.java" "ForEachTest" || true
test_compile_only "${SCRIPT_DIR}/ForEachArrayTest.java" || true

echo -e "${BLUE}=== Testing Generics ===${NC}"
test_compile_only "${SCRIPT_DIR}/GenericBox.java" || true
test_compile_and_run "${SCRIPT_DIR}/GenericTest.java" "GenericTest" || true
test_compile_only "${SCRIPT_DIR}/SimpleGeneric.java" || true
test_compile_only "${SCRIPT_DIR}/TwoTypeParams.java" || true
test_compile_only "${SCRIPT_DIR}/BoundedGeneric.java" || true

# Cleanup
rm -f /tmp/jikes_*_$$.log

echo "================================================"
echo "  Test Summary"
echo "================================================"
echo -e "Total tests:  $TESTS_RUN"
echo -e "${GREEN}Passed:       $TESTS_PASSED${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Failed:       $TESTS_FAILED${NC}"
else
    echo -e "Failed:       $TESTS_FAILED"
fi
echo "================================================"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
else
    echo -e "${YELLOW}⚠ Some tests failed (this is okay if test files don't exist yet)${NC}"
    exit 1
fi
