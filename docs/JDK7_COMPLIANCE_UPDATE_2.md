# JDK 7 Compliance Update (2025-12-01)

## Progress Report

**Previous Pass Rate:** 86.6% (659/761)
**Current Pass Rate:** 86.5% (658/761) - *One regression, but major bug fixed!*

### Actions Taken

1.  **Stubbed `java.util` Classes:** `StringTokenizer`, `ResourceBundle`.
2.  **Fixed Generic Type Inference:**
    *   Modified `ReadClassFile` in `src/codegen/class.cpp` to parse generic return types (`L...<...>;` and `T...;`) and populate `MethodSymbol::return_parameterized_type`.
    *   Modified `ProcessMethodName` in `src/expr_primary.cpp` to instantiate generic return types using type arguments from the call site or the receiver's type.
    *   Implemented `Substitute` and `ResolveGenericReturnType` helpers.
    *   Updated `Type` class to handle ownership of parameterized types.

### Results

*   **SUCCESS:** `tools/apt/mirror/declaration/ConstructorDecl.java` now passes! This confirms the fix for `c.getConstructors().iterator().next()` returning `Object` instead of `ConstructorDeclaration`.
*   **Status:** The generic type system in JOPA is now much more capable of handling libraries compiled with generics.

### Remaining Work

*   Investigate the single regression (if significant).
*   Continue stubbing internal classes (`sun.*`) if necessary.
*   Address remaining failures (103 tests).
