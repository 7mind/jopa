# JDK 7 Compliance Failure Analysis & Roadmap

**Date:** 2025-12-03
**Status:** 85.9% Pass Rate on tools/javac (523/609), 72.1% Overall (549/761)
**Remaining Failures:** 86 (tools/javac), 212 (overall including javadoc stub failures)

## 1. Failure Categories (Detailed Analysis)

### 1.1 Missing API Stubs (34 tests) - Infrastructure
**Impact:** Medium (Affects specific tool tests)
**Description:** Tests for `javadoc` and `javac` API fail because the runtime environment lacks the necessary internal API stubs.

| Category | Count | Root Cause |
|----------|-------|------------|
| javadoc tests | 21 | Missing `Tester` class, `com.sun.javadoc.*` package |
| javac API tests | 13 | Missing `JavacTaskImpl.enter(Iterable)`, other internal APIs |

**Example Errors:**
- `Type "Tester" was not found` (javadoc tests)
- `No applicable overload for "enter(Iterable)"` (TestJavacTask.java)
- `No accessible field named "descriptor"` (OverrideBridge.java)

### 1.2 Generic Type Inference Issues (30 tests) - Compiler Core
**Impact:** High (Affects real-world generic code)

| Subcategory | Count | Description |
|-------------|-------|-------------|
| Type inference | 10 | Generic method return types degrade to `Object` |
| Rare types | 4 | Nested generics with static type access |
| Wildcards | 3 | Wildcard capture in complex scenarios |
| Other generics | 13 | Conditional expressions, recursive bounds, parametric exceptions |

**Example Errors:**
- `T6650759a.java`: `testSet(java.lang.Object)` instead of `testSet(Integer)` - target type not propagated to nested calls
- `T6330931.java`: `Object` returned instead of `Foo` from `List<? extends Foo>.get(0)`
- `Conditional.java`: Ternary `cond ? a : b` returns `Object` instead of common interface type
- `T6650759f.java`: Recursive bounds `C<X extends D>` fail to propagate

### 1.3 Boxing with Generics (4 tests)
**Impact:** Medium
**Description:** Generic fields treated as `Object` cannot participate in boxing/compound assignment.

**Example Error:**
- `T6369051.java`: `value += 1` fails because `value` (generic type) is `Object`, not numeric

### 1.4 Multicatch (2 tests)
**Impact:** Low (Missing classfile API fields)
**Description:** Tests use `com.sun.tools.classfile` API that is incomplete in stubs.

### 1.5 Diagnostics (3 tests)
**Impact:** Low (Test infrastructure)
**Description:** Tests for compiler diagnostics require specific message formats.

### 1.6 Miscellaneous (19 tests)
**Impact:** Low to Medium (Various edge cases)

| Test | Issue |
|------|-------|
| `ConstValInlining.java` | Constant value inlining |
| `DuplicateImport.java` | Import handling |
| `ExceptionalFinally.java` | Exception flow |
| `accessVirtualInner/Main.java` | Inner class access |
| `importContext/anonPackage/Foo.java` | Anonymous package imports |
| `types/*.java` | Type conversion tests (4) |
| Others | Various edge cases |

---

## 2. Completed Work

### Phase 1a: Foreach Loop Type Extraction ✅ COMPLETE (2025-12-02)

**Problem:** Foreach loops over parameterized Iterables (e.g., `List<String>`) were treating the element type as `Object` instead of the actual type argument.

**Root Causes Fixed:**
1. **Type argument chaining through inheritance** (`body.cpp:743-757`): When traversing `List<E> → Collection<E> → Iterable<E>`, the recursive type argument substitution was using the wrong context. Fixed by passing `super_type` instead of `concrete_type` in recursive calls to `FindIterableTypeArgumentInSuper`.

2. **Class signature parsing** (`codegen/class.cpp`): Parameterized interface signatures were being parsed correctly but the stub runtime jar wasn't being rebuilt. Verified that signature generation now correctly outputs parameterized interfaces (e.g., `Ljava/lang/Iterable<TE;>;`).

3. **Unboxing + widening in foreach** (`body.cpp:998-1028`, `bytecode_stmt.cpp:3157-3182`):
   - Added `iterator_element_type` field to `AstForeachStatement` to store the actual element type from the iterator
   - Semantic analysis now validates unboxing+widening conversions (e.g., `Integer` → `int` → `float`)
   - Bytecode generation uses the stored element type for CHECKCAST before unboxing

**Tests Added:**
- `test/foreach/BoxedForeachTest.java`: Comprehensive test covering all boxing/unboxing/widening scenarios

**Results:**
- `boxing/BoxedForeach.java`: Now passes
- Primary test suite: 214/214 tests pass

### Phase 1b: Method Lookup on Type Variables ✅ COMPLETE (2025-12-02)

**Problem:** When a method type parameter `E extends B<?>` was used as a formal parameter type, method lookup on the variable treated it as `Object` instead of using the bound `B`.

**Root Cause Fixed:**
`ProcessMethodTypeParameters` in `decl.cpp:1046-1060` was not checking nested classes in the containing type when looking up bounds. For `<E extends B<?>>` where `B` is a nested class, the simple name lookup failed.

**Fix Applied:**
Added nested class lookup in the containing type and enclosing types when resolving method type parameter bounds:
```cpp
// Check nested classes within the containing type
if (!bound_type && containing_type)
{
    bound_type = containing_type -> FindTypeSymbol(bound_name);
}

// Check enclosing types for nested classes
if (!bound_type && containing_type)
{
    for (TypeSymbol* enclosing = containing_type -> ContainingType();
         enclosing && !bound_type;
         enclosing = enclosing -> ContainingType())
    {
        bound_type = enclosing -> FindTypeSymbol(bound_name);
    }
}
```

**Results:**
- `cast/6569057/T6569057.java`: Now passes
- Primary test suite: 214/214 tests pass
- Minor regression: `T6650759i.java` now fails due to complex recursive type bounds (`<U extends A<U, V>, V extends B<V>>`). This exposes a latent issue with type inference on recursive bounds. **UPDATE:** Now fixed by Phase 2!

### Phase 2: Target Type Inference ✅ COMPLETE (2025-12-02)

**Problem:** Generic methods with no arguments (e.g., `<T> T empty()`) couldn't be assigned to a typed variable (e.g., `String s = empty()`) because the type parameter defaulted to `Object`.

**Fix Applied:**

1. **Track uninferred type parameters** (`ast.h`): Added `needs_target_type_inference` flag to `AstMethodInvocation` to mark method calls where the return type is a type parameter that couldn't be inferred from arguments.

2. **Set flag during type inference** (`expr_primary.cpp:1190-1195`): After all argument-based inference attempts fail, set the flag to allow target type inference.

3. **Apply target type in variable initializers** (`init.cpp:28-58`): After processing the initializer expression, if it's a method call needing target type inference and the variable type is compatible, use the variable type as the inferred return type.

4. **Apply target type in assignments** (`expr_ops.cpp:3237-3267`): Same logic for regular assignment expressions.

**Results:**
- 6 tests now pass that were failing:
  - `generics/inference/6369605/T6369605a.java`
  - `generics/inference/6995200/T6995200.java`
  - `generics/inference/6359106/T6359106.java`
  - `generics/NameOrder.java`
  - `generics/odersky/Test4.java`
  - `generics/inference/6650759/T6650759i.java` (Phase 1b regression now fixed!)
- Primary test suite: 214/214 tests pass
- JDK7 compliance: 669/761 (87.9%)

### Phase 2b: Nested Parameterized Type Inference in Foreach ✅ COMPLETE (2025-12-02)

**Problem:** Foreach loops over `Map.entrySet()` were failing because the element type `Map.Entry<K,V>` wasn't being properly inferred. The pattern `for (Map.Entry<String, Integer> e : map.entrySet())` was treating the element as `Object`.

**Root Cause:** The `entrySet()` method returns `Set<Map.Entry<K,V>>` - a nested parameterized type. When looking up `Map.Entry` as a nested class, the type table lookup was failing because nested classes use `$` notation internally (`java/util/Map$Entry`).

**Fix Applied:**
In `body.cpp`, added special handling for `entrySet()` to look up `Entry` through the containing type (`Map`) using `FindTypeSymbol()`:
```cpp
else if (strcmp(method_name, "entrySet") == 0)
{
    // Map.entrySet() returns Set<Map.Entry<K,V>>
    // Look up Entry via the containing type (Map)
    NameSymbol* entry_name = control.FindOrInsertName(
        (const wchar_t*)L"Entry", 5);
    TypeSymbol* entry_type = containing -> FindTypeSymbol(entry_name);
    if (entry_type)
    {
        component_type = entry_type;
        foreach -> iterator_element_type = component_type;
    }
}
```

**Results:**
- `for (Map.Entry<K,V> e : map.entrySet())` now compiles and runs correctly
- Primary test suite: 214/214 tests pass

### Phase 3a: Type Parameter Bound Improvements ✅ COMPLETE (2025-12-02)

**Problem:** Method type parameters with bounds from single-type imports (e.g., `import java.util.List`) weren't being resolved. Also, forward references in mutually recursive type parameters caused errors.

**Root Causes Fixed:**

1. **Single-type import lookup for type parameter bounds** (`decl.cpp:1063-1075`): Added lookup in `single_type_imports` before falling back to `ImportType`. This fixes cases like `<T extends List>` where `List` is from `import java.util.List`.

2. **Parameterized bound storage** (`decl.cpp:1138-1190`): For method type parameters with parameterized bounds like `<T extends List<? extends Number>>`, we now store the parameterized bound using `type_param->AddParameterizedBound()` and propagate it to formal parameters for correct method return type substitution.

3. **Forward reference handling** (`decl.cpp:1144-1180`): For mutually recursive method type parameters like `<T extends List<U>, U extends List<T>>`, added check to skip parameterized bound processing when the bound references not-yet-defined type parameters.

4. **Ternary operator LUB improvement** (`expr_ops.cpp:3188-3193`): When multiple common interfaces exist for a ternary expression, now returns the first most specific common interface instead of `Object`.

**Results:**
- Primary test suite: 214/214 tests pass
- JDK7 compliance: 671/761 (88.2%, +2 from baseline)

### Phase 3b: Intersection Type Support for Ternary ✅ COMPLETE (2025-12-02)

**Problem:** When a ternary expression's branches implement multiple common interfaces, assignment to any of those interfaces failed because the LUB was a single interface, not an intersection type.

**Example:**
```java
interface I {}
interface J {}
class A implements I, J {}
class B implements I, J {}
// J j = cond ? new A() : new B();  // Failed: LUB was I, not assignable to J
```

**Fix Applied:**
In `expr_ops.cpp` `CanAssignmentConvert()`, added intersection type support that checks if both branches of a conditional expression are individually assignable to the target type:
```cpp
AstConditionalExpression* conditional = expr -> ConditionalExpressionCast();
if (conditional)
{
    // Unwrap casts to get the original expression types
    // ... check if both branches are assignable to target_type
}
```

**Results:**
- `generics/Conditional.java`: Now passes
- JDK7 compliance: 672/761 (88.3%, +1)

### Phase 3c: Nested Method Call Type Inference ✅ COMPLETE (2025-12-02)

**Problem:** When a generic method result was passed as an argument to another method, type inference from bounded type parameters failed because argument expressions were wrapped in cast nodes before inference could extract the original type.

**Example:**
```java
<I extends Interface<T>, T> T getValue(I arg) { return null; }
void useInt(Integer i) { }
void test() {
    useInt(getValue(new IntImpl()));  // Failed: returned Object instead of Integer
}
```

**Root Cause:** Method argument type coercion wraps expressions in CAST nodes before secondary inference runs. The secondary inference code was calling `arg->Type()` which returned the cast target type (the formal parameter erasure) instead of the original expression type.

**Fix Applied:**
In `expr_primary.cpp` secondary inference block, added cast unwrapping to get the original expression's type:
```cpp
// Unwrap cast expressions to get the original expression's type
AstExpression* original_arg = arg;
while (original_arg -> kind == Ast::CAST)
{
    AstCastExpression* cast = (AstCastExpression*) original_arg;
    original_arg = cast -> expression;
}
TypeSymbol* original_type = original_arg -> Type();
if (original_type && original_type != arg_type)
    arg_type = original_type;
```

**Results:**
- `T6650759a.java`: Now passes (nested generic method calls)
- `T6650759l.java`: Now passes
- Primary test suite: 214/214 tests pass
- JDK7 compliance: 674/761 (88.6%, +2)

### Phase 3d: Inherited Generic Field Type Substitution ✅ COMPLETE (2025-12-03)

**Problem:** When a field is inherited from a generic superclass (e.g., `Value<T>`), accessing it from a subclass with concrete type arguments (e.g., `IntegerValue extends Value<Integer>`) returned the erased type (`Object`) instead of the substituted type (`Integer`).

**Example:**
```java
class Value<T> { public T value; }
class IntegerValue extends Value<Integer> {
    void increment() {
        value = value + 1;  // Failed: value was Object, not Integer
    }
}
```

**Fix Applied:**
In `expr_names.cpp` `ProcessAmbiguousName()`, added type substitution logic after field lookup. When a field is inherited from a generic superclass, the code now walks the superclass chain to find the parameterized superclass and substitutes the type parameter with the actual type argument.

**Results:**
- `T6369051.java`: Now passes
- JDK7 compliance: 675/761 (88.7%, +1)

### Phase 3e: Wildcard Capture for Type Arguments ✅ COMPLETE (2025-12-03)

**Problem:** When calling methods on types with wildcard type arguments (e.g., `FooList<? super Bar>`), the return type was `Object` instead of the correct upper bound from the type parameter's declaration.

**Example:**
```java
interface FooList<T extends Foo> extends List<T> {}
<T extends FooList<? super Bar>> void m(T t) {
    Foo f = t.get(0);  // Failed: get returned Object instead of Foo
}
```

**Root Causes Fixed:**

1. **Missing wildcard type storage in `ProcessTypeArguments`** (`decl.cpp:1346-1380`): When creating `ParameterizedType` for type arguments, wildcards were not being converted to `WildcardType` objects. Instead, their erasure (`Object`) was stored. Fixed by detecting `AstWildcard` and creating proper `WildcardType` objects with the correct `BoundKind` (EXTENDS, SUPER, or UNBOUNDED).

2. **Wildcard capture in type substitution** (`expr_primary.cpp:755-799`): When substituting type arguments for method return types, wildcards with `? super X` or unbounded `?` were returning `Object`. Fixed by checking if the substituted type argument is a wildcard, and if so, using the corresponding type parameter's upper bound instead of the wildcard's erasure.

**Results:**
- `T6330931.java`: Now passes (wildcard with `? super` bounds)
- `T5097548b.java`: Now passes (unbounded wildcard with F-bounded type parameter)

### Phase 3f: Generic Signature Parsing from Class Files ✅ COMPLETE (2025-12-03)

**Problem:** When reading methods from class files that return parameterized types like `Set<Map.Entry<K,V>>`, the `return_parameterized_type` wasn't being constructed. This caused foreach loops to fail because the iterator element type couldn't be inferred.

**Example:**
```java
// In JarVerifier.java:
for (Entry<String, Attributes> me : entries.entrySet()) {
    // Failed: element type was Object, not Map.Entry<String,Attributes>
}
```

**Root Causes Fixed:**

1. **Field generic signature storage** (`symbol.h`, `decl.cpp`, `class.cpp`): Added `generic_signature` field to `VariableSymbol` to store type parameter references (format: `TV;`) for fields inherited from generic superclasses. This allows type substitution during field access.

2. **Signature attribute emission** (`bytecode_init.cpp`): Extended signature attribute emission to cover fields with type parameter types and methods with parameterized return types.

3. **Parameterized return type parsing** (`class.cpp:2721-2837`): Added code to parse complex parameterized return types from generic signatures. For signatures like `()Ljava/util/Set<Ljava/util/Map$Entry<TK;TV;>;>;`, the code now:
   - Extracts the base type name and resolves it via `ReadTypeFromSignature`
   - Parses type arguments including type parameter references (`TK;`, `TV;`)
   - Constructs `ParameterizedType` with proper type argument references

**Fix Applied:**
```cpp
// Parse the base type from the signature (between 'L' and '<')
const char* type_name_start = p + 1; // skip 'L'
int type_name_len = search - type_name_start;
TypeSymbol* base_type = ReadTypeFromSignature(
    type, type_name_start, type_name_len, tok);
```

**Results:**
- GNU Classpath bootstrap: COMPLETE (all files compile)
- JarVerifier.java and similar Map.entrySet() patterns: Now work correctly
- Primary test suite: 214/214 tests pass

---

## 3. Bootstrap Build Verification ✅ COMPLETE (2025-12-02)

| Component | Status | Notes |
|-----------|--------|-------|
| GNU Classpath 0.99 | ✅ PASS | All Java bytecode compiled by JOPA |
| JamVM 2.0.0 | ✅ PASS | classes.zip compiled by JOPA |
| Apache Ant 1.8.4 | ✅ PASS | Bootstraps on JamVM |
| ECJ 4.2.1 | ✅ PASS | 414 classes, runs on JamVM |

The ECJ build target has been added to `vendor/CMakeLists.txt`. ECJ 4.2.1 compiles with JOPA (excluding Ant integration and JSR 199/269 tool packages which depend on APIs not in GNU Classpath). The resulting `ecj.jar` (~1.5MB with resources) runs on JamVM and can compile Java code.

---

## 4. Roadmap to 95%+ Compliance

### Phase 4: Remaining Type Inference Issues (High Impact - ~20 tests)
**Goal:** Fix remaining generic type inference edge cases
**Status:** In Progress
**Priority:** HIGH

**Current Failure Breakdown (tools/javac only - 93 failures):**

| Category | Count | Description |
|----------|-------|-------------|
| Multi-file tests | 13 | Need sourcepath compilation support |
| Missing ToolTester stub | 12 | Test infrastructure stub |
| Method overload resolution | 6 | Generic method signature matching |
| Missing List import | 4 | Stub runtime needs java.util.List |
| Internal javac API | 3 | com.sun.tools.javac.code.* stubs |
| Rare types (static nested) | 4 | Static nested classes in generics |
| Other edge cases | ~51 | Various specific issues |

**Key issues to address:**
1. **Rare types** (`Rare8.java`, `Rare9.java`, `Rare10.java`, `Rare11.java`): Static nested classes in generic contexts require qualified type names
2. **Parametric exceptions** (`ParametricException.java`): Generic exception type inference
3. **Recursive bounds** (`T6650759f.java`): Complex F-bounded polymorphism
4. **Bridge methods** (`OverrideBridge.java`): Bridge method visibility issues

### Phase 5: Expand API Stubs (Medium Impact - ~115 tests)
**Goal:** Make javadoc and tool tests pass
**Status:** Not started
**Priority:** MEDIUM (infrastructure work, doesn't affect real compilation)

**Approach:**
1. **`runtime/com/sun/javadoc`**: Add `Tester` class and javadoc API (~103 tests affected)
2. **`runtime/com/sun/tools/javac/api`**: Add/Update `JavacTask`, `JavacTaskImpl` with correct signatures
3. **`runtime/com/sun/tools/classfile`**: Add missing fields (`descriptor`, `exception_table_length`, etc.)
4. **Test infrastructure**: Add `ToolTester`, `Example`, etc. for test harness

### Phase 6: Multi-file Compilation Tests (Low Impact - ~13 tests)
**Goal:** Fix tests that require sourcepath compilation
**Status:** Not started
**Priority:** LOW

**Approach:**
1. Either add sourcepath support to test runner, or
2. Manually verify these tests work with proper sourcepath setup

---

## 5. Summary

**Current Status:**
- **tools/javac tests:** 516/609 (84.7%)
- **Overall (including javadoc):** 542/761 (71.2%)
- **Primary test suite:** 214/214 (100%)
- **Bootstrap:** COMPLETE

| Category | Tests | Priority | Notes |
|----------|-------|----------|-------|
| Type Inference | ~20 | HIGH | Real compiler bugs |
| API Stubs (javadoc) | ~103 | MEDIUM | Infrastructure only |
| API Stubs (other) | ~12 | MEDIUM | ToolTester, etc. |
| Multi-file tests | 13 | LOW | Sourcepath setup |
| Misc | ~20 | LOW | Edge cases |

**Recently Fixed (Phase 3d, 3e, 3f, 4a):**
- ✅ Inherited generic field type substitution (`T6369051.java`)
- ✅ Wildcard capture for `? super` and unbounded `?` (`T6330931.java`, `T5097548b.java`)
- ✅ Parameterized return type parsing from class files (GNU Classpath bootstrap)
- ✅ Generic signature storage for inherited fields with type parameters
- ✅ Added `java.util.Queue`, `java.util.Deque`, `java.util.LinkedHashSet` stubs
- ✅ Added `ToolTester` test infrastructure stub (+7 tests passing)

**Next Steps (Recommended Order):**
1. **Rare types** - Fix static nested class access in generic contexts (4 tests)
2. **Method overload** - Fix generic method signature matching (6 tests)
3. **Stub expansion** - Add java.util.List, Queue, LinkedHashSet to stubs (4 tests)
4. **ToolTester stub** - Add test harness classes (12 tests)

**Bootstrap Status:** COMPLETE - JOPA can compile GNU Classpath, JamVM, Apache Ant, and ECJ from source, creating a fully self-contained Java development environment with no binary blobs.
