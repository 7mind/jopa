# JDK 7 Compliance: Generics Failure Analysis

**Date:** 2025-12-03
**Scope:** `tools/javac/generics/`
**Status:** 77 failing tests remaining (down from 80+).

## 1. Missing API Stubs

Several failures were due to missing methods in the Jopa runtime stubs (`jopa-stub-rt.jar`).

**Fixed:**
*   `java.util.Collections`: Added `reverseOrder()`, `fill()`, `sort()`.
*   `java.util.Arrays`: Added `fill()` overloads, `copyOf()`.
*   `java.lang.Class`: Added `cast(Object)`.
*   `com.sun.tools.classfile.Descriptor`: Added `getReturnType()`, `getParameterTypes()`.
*   `com.sun.tools.classfile.ClassFile`: Added `getName()`.
*   `java.awt.Color`: Created dummy class.

**Remaining Issues:**
*   `T6507024.java`: `Class.cast()` return type inference. `getClass()` in `Object` is declared as `Class<?>` in stubs, but should behave as `Class<? extends |T|>` (compiler magic). Jopa currently infers `Object` for `cast` return, causing assignment failure.

## 2. Generic Method Overriding & Covariance

**Representative Test:** `T6294779b.java`
**Error:** `In type "T6294779b$I4", the method "java.util.List m();", inherited from type "T6294779b$I1", does not have the same return type as the method "java.util.LinkedList m();", inherited from type "T6294779b$I3.`
**Analysis:**
The interface `I4` inherits `m()` from `I1` (returns `List`), `I2` (returns `Queue`), and `I3` (returns `LinkedList`).
Since `LinkedList` implements both `List` and `Queue`, it is the most specific return type and should satisfy all overrides.
Jopa fails to recognize this covariant override relationship in the context of multiple interface inheritance, likely performing a strict equality check or failing to identify `LinkedList` as the bridge/implementation target.

**Related Tests:**
*   `T4711694.java`: Abstract method implementation check failing for generic method override.

## 3. Type Inference

**Representative Test:** `ParametricException.java`
**Error:** `The method "accept(...)" can throw the checked exception "java.lang.Throwable", so its invocation must be enclosed in a try statement...`
**Analysis:**
The method `accept` declares `throws E`. The type parameter `E` should be inferred as `IOException` (from context), but Jopa infers it as `Throwable` (the upper bound). This suggests an issue with exception type inference or constraint propagation.

**Other Inference Failures:**
*   `NewTest.java`: Return type inference yields `Number` instead of `List` in complex nested generic call.
*   `T6938454a.java`: Return type mismatch in generic method chain.
*   `T5034571.java`: Intersection type method lookup failure (`i1` not found in `I1 & I2` variable where `I1` has `i1`).

## 4. Static Access to Inner Classes

**Representative Test:** `Rare8.java`
**Error:** `The static type "rare8.C$D" must use a qualified name to access the non-static member type "rare8.A$B" of the enclosing type "rare8.C".`
**Analysis:**
`C` extends `A`. `A` has inner class `B`. `C` has static nested class `D`.
`D` tries to refer to type `B` (inherited by `C` from `A`).
Jopa incorrectly enforces "non-static member" access rules on the *type name* `B`, requiring qualification. Since `B` is a type, it should be accessible by simple name in inherited contexts, even static ones (as a type), unless JLS rules for member type hiding/access are interpreted strictly differently.

## 5. Test Environment & Source Structure

**Representative Test:** `RetroLexer.java`
**Error:** `The import "util.Hashtable" is not valid...`
**Analysis:**
The test files (`RetroLexer.java`, `Hashtable.java`, `CharScanner.java`) are all in the same directory (`generics/rawSeparate`) but declare different packages (`parser`, `util`, `antlr`).
Jopa's compilation (via `test-compliance.sh`) adds the directory to the sourcepath, but Jopa expects sourcepath entries to follow the package directory structure (e.g., `util/Hashtable.java`).
**Fix Strategy:** Compile these files in batch mode (all on command line) or restructure the test directory for compliance testing.

## 6. Resolved Issues (Pollution)

*   `BridgeRestype.java` and `ParenVerify.java` were failing due to a stray `Iterator.class` file in the `generics/` directory (leftover from `InnerInterface1.java`). Removing the pollution fixed these tests.