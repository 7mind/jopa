# JDK 7 Compliance Roadmap & Log

**Status:** 86.9% Pass Rate (661/761)
**Remaining Failures:** 100

## Progress Log

### Phase 1: Fix Member Lookup & Substitution
*   [x] **Debug `BoxedForeach.java`**: Confirmed `Iterable<T>` resolution issue.
*   [x] **Fix `ProcessForeachStatement`**:
    *   Implemented breadth-first search with type substitution.
    *   Propagated `parameterized_type` in `ProcessAmbiguousName` and `ProcessFieldAccess`.
    *   Implemented parameterized interface parsing in `ProcessClassFile` (ClassReader) to support binary dependencies.
    *   **Result:** `jopa-stub-rt` errors resolved.

### Phase 2: Remaining Semantic & Inference Issues
*   [x] **Fix Unboxing+Widening in Foreach**:
    *   Updated `CanMethodInvocationConvert` to allow unboxing followed by widening (JLS 5.3).
    *   Implemented `GetIterableElementType` in `Semantic` to correctly identify element type from parameterized iterables.
    *   Updated `EmitForeachStatement` to cast to the correct element type before unboxing/widening.
    *   **Result:** `BoxedForeach.java` passes (and 4 other tests).
*   [ ] **Fix Type Inference**: Many failures in `tools/javac/generics/inference/` suggest issues with type inference for generic methods (JLS 15.12.2.7).
*   [ ] **Fix Enum Switch**: `tools/javac/enums/EnumSwitch.java` (if failing) or similar.
*   [ ] **Investigate API/Annotation Failures**: `tools/javac/api/` failures need classification.

### Phase 3: Polish & Optimization
*   Clean up debug prints.
*   Optimize type search performance.