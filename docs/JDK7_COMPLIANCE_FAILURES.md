# JDK 7 Compliance Failure Analysis Report

**Date:** 2025-12-01
**Scope:** Remaining failures in `scripts/test-java7-compliance.sh`
**Total Failures:** 108 files

## Executive Summary
The JDK 7 compliance suite currently has 108 failing tests out of 761 whitelisted files (85.8% pass rate). The failures generally fall into systematic categories rather than random logic errors. Addressing these categories would significantly boost the pass rate.

## Detailed Failure Categories

### 1. Generic Type Inference & Erasure (High Priority)
A recurring issue involves the compiler losing generic type information, likely due to overly aggressive raw type erasure or failure to track upper bounds during inference.
*   **Symptom:** `Semantic Warning: Conversion ... bypasses type parameterization`.
*   **Result:** Expressions are typed as `java.lang.Object` instead of the expected generic type, causing subsequent method calls to fail.
*   **Example:** `tools/javac/generics/wildcards/6330931/T6330931.java`
    *   `t.get(0)` returns `Object` instead of `Foo`.
*   **Impact:** Affects many tests in `tools/javac/generics/`.

### 2. Multi-file Compilation / Source Lookup
The current test runner compiles files individually. Some tests rely on package-private classes defined in *other* source files where the file name does not match the class name. Standard `javac` might handle this if compiled together or if the source path lookup strategy is more flexible, but our strict one-file-at-a-time approach fails here.
*   **Symptom:** `Type "X" was not found` where `X` is defined in a sibling file `Y.java`.
*   **Example:** `tools/javac/T6180021/AbstractSub.java` fails to find `AbstractSuper` (defined in `Super.java`).
*   **Impact:** Falsely flags valid code as failing due to test harness limitations.

### 3. Compiler Crashes (Critical)
There are assertion failures causing the compiler to abort. These are critical bugs.
*   **Symptom:** `Assertion failed` and exit code 134.
*   **Example:** `tools/javac/types/CastTest.java`
    *   `Assertion s[-2] == U_LB && s[-1] == U_RB' failed` in `Jopa::MethodSymbol::Header()`.
*   **Impact:** Stability risk.

### 4. Missing Runtime Library Classes (Standard & Internal)
Despite the exclusions, some tests still slip through or rely on parts of the runtime we haven't stubbed yet.
*   **Standard Lib:** `java.util.zip` implementation details, `java.util.ResourceBundle` (in some diagnostics tests).
*   **Internal JDK:** `sun.tools.*`, `com.sun.tools.javac.*`, `com.sun.javadoc.*`.
    *   Tests often try to instantiate the compiler or javadoc tool programmatically.
*   **Example:** `tools/javadoc/annotations/elementTypes/Main.java` fails finding `Tester$Doclet`.

## Recommendations

1.  **Investigate "Bypasses Type Parameterization":** This warning seems to correlate strongly with broken type inference. Review `src/semantic.h` or `src/lookup.cpp` to see why it forces erasure to `Object`.
2.  **Fix Crashes:** Prioritize debugging `tools/javac/types/CastTest.java` to fix the assertion failure in `MethodSymbol::Header`.
3.  **Enhance Test Runner:** Modify `scripts/test-compliance.sh` to detect if a test directory contains multiple dependent `.java` files and compile them together, or verify if the "missing" classes are indeed in sibling files.
4.  **Expand Stubs:** Continue filling in `jopa-stub-rt.jar` for high-value missing classes like `java.util.zip` entries if we want to pass `tools/javac/file/zip` tests.
