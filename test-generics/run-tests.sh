#!/usr/bin/env bash
# Test script for Jikes Java 5 Generics Implementation

set -e

JIKES="../src/jikes"
RUNTIME_PATH="runtime"
OUTPUT_DIR="."

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counter
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

echo "========================================="
echo "Jikes Java 5 Generics Test Suite"
echo "========================================="
echo ""

# Function to run a test
run_test() {
    local test_name="$1"
    local test_file="$2"

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    echo -n "Testing $test_name... "

    if $JIKES -sourcepath $RUNTIME_PATH -d $OUTPUT_DIR "$test_file" 2>&1 | grep -q "error"; then
        echo -e "${RED}FAILED${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        $JIKES -sourcepath $RUNTIME_PATH -d $OUTPUT_DIR "$test_file" 2>&1 | head -20
        return 1
    else
        echo -e "${GREEN}PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    fi
}

# Clean previous builds
echo "Cleaning previous builds..."
rm -f *.class
echo ""

# Check if jikes exists
if [ ! -f "$JIKES" ]; then
    echo -e "${RED}ERROR: Jikes binary not found at $JIKES${NC}"
    echo "Please build Jikes first: cd ../src && make"
    exit 1
fi

echo "Using Jikes: $JIKES"
echo ""

# Run tests
echo "Running tests..."
echo "----------------------------------------"

run_test "Simple Generic Class" "SimpleGeneric.java"
run_test "Multiple Type Parameters" "TwoTypeParams.java"
run_test "Bounded Type Parameters" "BoundedGeneric.java"

# Test compilation of all files together
echo ""
echo "Testing batch compilation..."
rm -f *.class
if $JIKES -sourcepath $RUNTIME_PATH -d $OUTPUT_DIR SimpleGeneric.java TwoTypeParams.java BoundedGeneric.java 2>&1 | grep -q "error"; then
    echo -e "${RED}Batch compilation FAILED${NC}"
    FAILED_TESTS=$((FAILED_TESTS + 1))
else
    echo -e "${GREEN}Batch compilation PASSED${NC}"
    PASSED_TESTS=$((PASSED_TESTS + 1))
fi
TOTAL_TESTS=$((TOTAL_TESTS + 1))

# Summary
echo ""
echo "========================================="
echo "Test Summary"
echo "========================================="
echo "Total tests:  $TOTAL_TESTS"
echo -e "Passed:       ${GREEN}$PASSED_TESTS${NC}"
if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "Failed:       ${RED}$FAILED_TESTS${NC}"
else
    echo -e "Failed:       $FAILED_TESTS"
fi
echo ""

# Verify bytecode if any class compiled
if [ -f "SimpleGeneric.class" ]; then
    echo "Verifying bytecode..."
    echo "SimpleGeneric.class signature:"
    od -c SimpleGeneric.class | grep -A 2 "Signature" | head -5 || echo "(no javap available, using od)"
fi

echo ""

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    exit 1
fi
