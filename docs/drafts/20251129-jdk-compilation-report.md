# JDK Compilation Test Report

Date: 2025-11-29

## Overview

This report analyzes JOPA's ability to compile JDK 7/8 langtools test files
without the `--parse-only` flag. Previously, these tests only tested parsing.

## Raw Results

| JDK Version | Total Tests | Compiled | Failed | Timeout |
|-------------|-------------|----------|--------|---------|
| JDK 7       | 3029        | 1398     | 1631   | 0       |
| JDK 8       | 4501        | 1763     | 2649   | 89      |
| **Total**   | **7530**    | **3161** | **4280**| **89**  |

Raw compilation success rate: 42.0%

## Adjusted Analysis

The JDK test suite includes three types of tests:
- `@compile` - Tests that should compile successfully
- `@compile/fail` - Tests that should fail compilation (negative tests)
- `@run` - Tests that should compile and run

### JDK 7 Annotated Tests

| Annotation      | Compiled | Failed | Correct Outcome |
|-----------------|----------|--------|-----------------|
| @compile/fail   | 271      | 328    | Failed is correct |
| @compile        | 397      | 277    | Compiled is correct |
| @run            | 47       | 240    | Compiled is correct |
| No annotation   | 683      | 786    | (Excluded from metrics) |

**JDK 7 Accuracy for annotated tests:**
- Correct: 397 + 47 + 328 = **772**
- Incorrect: 271 + 277 + 240 = **788**
- Accuracy: **49.5%**

### JDK 8 Annotated Tests

| Annotation      | Compiled | Failed | Correct Outcome |
|-----------------|----------|--------|-----------------|
| @compile/fail   | 314      | 591    | Failed is correct |
| @compile        | 520      | 492    | Compiled is correct |
| @run            | 119      | 478    | Compiled is correct |
| No annotation   | 810      | 1088   | (Excluded from metrics) |

**JDK 8 Accuracy for annotated tests:**
- Correct: 520 + 119 + 591 = **1230**
- Incorrect: 314 + 492 + 478 = **1284**
- Accuracy: **48.9%**

### Combined Accuracy

- Correct: 772 + 1230 = **2002**
- Incorrect: 788 + 1284 = **2072**
- **Overall Accuracy: 49.2%**

## Error Categories

### JDK 7 Failures by Test Category

| Category          | Failures | Passes | Pass Rate |
|-------------------|----------|--------|-----------|
| tools/javac/diags | 210      | 153    | 42.1%     |
| tools/javac/generics | 196   | 292    | 59.8%     |
| tools/javac/processing | 99  | 39     | 28.3%     |
| tools/javac/api   | 83       | 7      | 7.8%      |
| tools/javac/DefiniteAssignment | 34 | 21 | 38.2%  |
| tools/javac/multicatch | 21  | 9      | 30.0%     |
| tools/javac/enum  | 21       | 39     | 65.0%     |
| tools/javac/annotations | 20 | 57     | 74.0%     |

### JDK 8 Failures by Test Category

| Category          | Failures | Passes | Pass Rate |
|-------------------|----------|--------|-----------|
| tools/javac/lambda | 376     | 22     | 5.5%      |
| tools/javac/diags | 263      | 200    | 43.2%     |
| tools/javac/generics | 223   | 325    | 59.3%     |
| tools/javac/annotations | 189 | 101   | 34.8%     |
| tools/javac/processing | 143 | 71     | 33.2%     |
| tools/javac/api   | 90       | -      | ~0%       |
| tools/javac/defaultMethods | 36 | 20  | 35.7%     |

### Timeout Tests (89 total)

Tests that cause infinite loops in the compiler (mostly lambda/method reference related):
- `tools/javac/T8022316/CompilerErrorGenericThrowPlusMethodRefTest.java`
- `tools/javac/T8023112/SkipLazyConstantCreationForMethodRefTest.java`
- Various lambda diagnostic tests

## Key Findings

1. **Generics**: ~60% pass rate - reasonably good support
2. **Enums**: ~65% pass rate - good support
3. **Annotations**: 34-74% pass rate depending on complexity
4. **Lambda expressions**: ~5.5% pass rate - minimal support (expected)
5. **Diagnostic tests**: ~42-43% pass rate - many are negative tests
6. **API tests**: ~0-8% pass rate - likely missing compiler API stubs
7. **Processing tests**: ~28-33% pass rate - annotation processing issues

## Recommendations

1. **Lambda support**: The 89 timeout tests indicate infinite loops in lambda/method reference handling
2. **Diagnostic tests**: Many failures are intentional negative tests that we correctly reject
3. **API tests**: Need compiler API stubs in jopa-stub-rt
4. **Helper files**: Many "NO_ANNOTATION" failures are helper classes that need dependencies

## Files Location

- JDK7 failed tests: `/tmp/jdk_compile_test_23079/jdk7_failed.txt`
- JDK8 failed tests: `/tmp/jdk_compile_test_23079/jdk8_failed.txt`
- JDK8 timeout tests: `/tmp/jdk_compile_test_23079/jdk8_timeout.txt`
