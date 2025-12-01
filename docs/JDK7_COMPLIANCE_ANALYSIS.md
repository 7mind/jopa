# JDK 7 Compliance Failure Analysis

**Date:** 2025-12-01
**Scope:** JDK 7 Compliance Test Suite (`scripts/test-java7-compliance.sh`)

## Executive Summary
The current compliance pass rate is low largely due to a significantly incomplete runtime library (`jopa-stub-rt.jar`). The failures are rarely due to compiler logic errors but rather the absence of types and methods required by the test cases.

## Failure Categories

### 1. Missing Standard Library Classes
A significant number of failures are caused by missing classes in the `runtime/` directory. The `jopa` runtime is a minimal stub, and many tests rely on standard Java classes that have not yet been stubbed.

**Examples:**
- `java.util.StringTokenizer` (Missing)
- `java.util.ResourceBundle` (Missing)
- `java.net.URI` (Exists but missing `toURL()`, `getScheme()`, etc.)
- `java.lang.Class` (Missing generics support or specific methods)

**Impact:**
- `tools/javac/diags/*` tests fail heavily due to missing `ResourceBundle` and `StringTokenizer`.
- `tools/javac/file/zip/*` tests fail due to incomplete `java.io` and `java.net` support.

### 2. Missing Internal JDK Classes (`sun.*`, `com.sun.*`)
The JDK tests often rely on internal implementation classes which are not part of the public API but are present in a standard JDK. These are missing from our test environment/runtime.

**Examples:**
- `sun.tools.jar.Main`
- `com.sun.tools.javac.util.JCDiagnostic`
- `com.sun.tools.javac.util.JavacMessages`
- `com.sun.tools.javac.main.Main`

**Impact:**
- `tools/javac/api/*` and `tools/javac/diags/*` tests fail because they try to interact with the compiler's internal classes.

### 3. Generic Type Inference & Erasure Issues
There are semantic errors where the compiler fails to correctly infer types, causing it to treat expressions as `java.lang.Object`. This leads to "Method not found" errors when the code attempts to call methods on the result.

**Examples:**
- `tools/apt/mirror/declaration/ConstructorDecl.java`: `c.getConstructors().iterator().next()` is treated as `Object` instead of `ConstructorDeclaration`.
- `tools/javac/generics/inference/5034571/T5034571.java`: Method lookups fail on inferred types.

**Root Cause:**
Likely issues in `src/lookup.cpp` or `src/semantic.h` regarding raw type conversion warnings (`bypasses type parameterization`) which might be treated too aggressively or incorrectly, leading to type information loss.

### 4. Annotation Processing / API Stubs
Tests in `tools/apt` and `tools/javac/api` fail due to mismatches between the expected API (JSR 199, JSR 269) and the stubs or implementation provided.

**Examples:**
- `tools/javac/api/TestJavacTask.java`: Missing overload `enter(Iterable)`.
- `tools/apt/mirror/util/Overrides.java`: Failures in method overriding checks.

## Recommendations

1.  **Expand Runtime Stubs:** Prioritize adding missing `java.util` and `java.io` classes (`StringTokenizer`, `ResourceBundle`) and filling in missing methods in `java.net.URI`.
2.  **Stub Internal Classes:** Create minimal stubs for `sun.tools.jar.Main` and core `com.sun.tools.javac` classes if we intend to pass these specific tests, or whitelist them out if they are deemed out of scope for a non-HotSpot compiler.
3.  **Investigate Type Inference:** Debug the "bypasses type parameterization" warnings to ensure they aren't causing the compiler to prematurely discard generic type information.
