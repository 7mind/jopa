# Fix Report: JDK 7 Compliance Category 1.3 (Intersection Bounds)

## Status
**Category 1.3 Fixed:** `T4856983.java` (Intersection Bounds) now PASSES.
**Pass Rate:** 78.8% (Dropped from 86.5%). 600/761 tests passing.

## Changes Implemented
1.  **Infrastructure:**
    *   Modified `TypeSymbol` to support `TYPE_VARIABLE` kind.
    *   Added `TypeParameterSymbol* type_parameter` field to `TypeSymbol`.
    *   Added `ACC_TYPE_VARIABLE` flag.
2.  **Logic:**
    *   Implemented `TypeSymbol::IsSubtype` to correctly handle Type Variables by checking *all* bounds (intersection logic) instead of just the erasure.
    *   Updated `FindType` in `src/decl.cpp` to return a `TypeSymbol` wrapper around the `TypeParameterSymbol` instead of the erased type (Object/Bound).
    *   Updated `TypeSymbol` member lookup (`FindMethodSymbol`, etc.) to delegate to the erasure (super) for Type Variables.
    *   Updated `TypeSymbol::Erasure()` to return the erasure (super) for Type Variables.
    *   Updated `RegisterClass` in `src/codegen/bytecode.h` to explicitly erase Type Variables before registration.
    *   Updated `TypeSymbol::SignatureString()` to delegate to the erasure for Type Variables (fixes crash in codegen).
    *   Updated `Semantic::CheckMethodOverride` (covariant return type check) to allow Type Variables if they are subtypes of the overridden return type (or if the overridden return type is Object, or if the type variable is a subtype).

## Known Issues & Regressions
The drop in pass rate is due to the architectural shift of `FindType` returning Type Variables instead of Erasure.
*   **Semantic Regressions:** Many tests fail because they expect `FindType` to return a Class/Interface and perform operations that are not yet fully delegated or handled for Type Variables.
*   **Codegen Regressions:** Although `RegisterClass` and `SignatureString` are patched, there might be other paths in codegen that choke on Type Variables.

## Next Steps
1.  Systematically address regressions by identifying where Type Variables are being used as Classes.
2.  Fix Category 1.1 (Aggressive Erasure) which is the root cause of many other failures.
