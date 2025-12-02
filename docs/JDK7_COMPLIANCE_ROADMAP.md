# JDK 7 Compliance Roadmap & Log

**Status:** 86.2% Pass Rate (656/761)
**Remaining Failures:** 105

## Progress Log

### Phase 1: Fix Member Lookup & Substitution
*   [x] **Debug `BoxedForeach.java`**: Confirmed `Iterable<T>` resolution issue.
*   [x] **Fix `ProcessForeachStatement`**:
    *   Implemented breadth-first search with type substitution.
    *   Propagated `parameterized_type` in `ProcessAmbiguousName` and `ProcessFieldAccess`.
    *   Implemented parameterized interface parsing in `ProcessClassFile` (ClassReader) to support binary dependencies.
    *   **Result:** `jopa-stub-rt` errors resolved.

### Phase 2: Remaining Semantic & Inference Issues
*   [ ] **Fix Unboxing+Widening in Foreach**: `tools/javac/boxing/BoxedForeach.java` fails when iterating `Byte[]` to `float` (Unboxing `Byte` -> `byte` -> Widening `float`).
*   [ ] **Fix Type Inference**: Many failures in `tools/javac/generics/inference/` suggest issues with type inference for generic methods (JLS 15.12.2.7).
*   [ ] **Fix Enum Switch**: `tools/javac/enums/EnumSwitch.java` (if failing) or similar.
*   [ ] **Investigate API/Annotation Failures**: `tools/javac/api/` failures need classification.

### Phase 3: Polish & Optimization
*   Clean up debug prints.
*   Optimize type search performance.