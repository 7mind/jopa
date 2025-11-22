# Autoboxing/Unboxing Implementation Plan

**Feature**: Automatic conversion between primitive types and wrapper classes
**JSR**: JSR 201
**Priority**: HIGH
**Estimated Complexity**: Medium-High
**Estimated Time**: 2-3 weeks

## Overview

Autoboxing and unboxing allow automatic conversion between primitive types and their corresponding wrapper classes:
- `int` ↔ `Integer`
- `boolean` ↔ `Boolean`
- `byte` ↔ `Byte`
- `char` ↔ `Character`
- `short` ↔ `Short`
- `long` ↔ `Long`
- `float` ↔ `Float`
- `double` ↔ `Double`

## Use Cases

### 1. Assignment
```java
Integer i = 42;        // Boxing: int → Integer
int j = i;             // Unboxing: Integer → int
```

### 2. Method Arguments
```java
void foo(Integer x) { }
foo(42);               // Boxing: int → Integer

void bar(int x) { }
Integer i = 42;
bar(i);                // Unboxing: Integer → int
```

### 3. Return Values
```java
Integer getValue() {
    return 42;         // Boxing: int → Integer
}

int compute() {
    Integer i = 42;
    return i;          // Unboxing: Integer → int
}
```

### 4. Operators (Unboxing)
```java
Integer a = 10, b = 20;
int sum = a + b;       // Unboxing: Integer → int for arithmetic
```

### 5. Generics (Critical)
```java
List<Integer> list = new ArrayList<Integer>();
list.add(42);          // Boxing: int → Integer
int x = list.get(0);   // Unboxing: Integer → int
```

## Implementation Strategy

### Phase 1: Type Mapping and Utilities

**File**: `src/control.h` or new `src/boxing.h`

Create primitive ↔ wrapper mapping:
```cpp
class BoxingUtility {
public:
    static TypeSymbol* GetWrapperType(TypeSymbol* primitive);
    static TypeSymbol* GetPrimitiveType(TypeSymbol* wrapper);
    static bool IsBoxingConversion(TypeSymbol* from, TypeSymbol* to);
    static bool IsUnboxingConversion(TypeSymbol* from, TypeSymbol* to);
    static NameSymbol* GetValueOfMethod(TypeSymbol* wrapper);
    static NameSymbol* GetUnboxMethod(TypeSymbol* wrapper);
};
```

Mapping table:
- `int` → `java.lang.Integer` (valueOf, intValue)
- `boolean` → `java.lang.Boolean` (valueOf, booleanValue)
- `byte` → `java.lang.Byte` (valueOf, byteValue)
- `char` → `java.lang.Character` (valueOf, charValue)
- `short` → `java.lang.Short` (valueOf, shortValue)
- `long` → `java.lang.Long` (valueOf, longValue)
- `float` → `java.lang.Float` (valueOf, floatValue)
- `double` → `java.lang.Double` (valueOf, doubleValue)

### Phase 2: Semantic Analysis Changes

**File**: `src/semantic.cpp`

#### 2.1 Type Compatibility
Modify `Semantic::CanAssignmentConvert()` and related methods:
```cpp
bool Semantic::CanAssignmentConvertReference(TypeSymbol* target_type,
                                             TypeSymbol* source_type)
{
    // Existing identity and widening reference checks...

    // Add boxing conversion
    if (source_type -> Primitive() &&
        BoxingUtility::IsBoxingConversion(source_type, target_type))
        return true;

    // Add unboxing conversion
    if (target_type -> Primitive() &&
        BoxingUtility::IsUnboxingConversion(source_type, target_type))
        return true;

    return false;
}
```

#### 2.2 Method Overload Resolution
Update `Semantic::FindMostSpecificMethod()` to consider boxing/unboxing:
- Phase 1: Exact match (no boxing)
- Phase 2: Boxing/unboxing allowed
- Phase 3: Widening + boxing/unboxing

#### 2.3 Binary Operator Handling
Update `Semantic::ProcessBinaryExpression()`:
- Unbox wrapper types before arithmetic operations
- Box result if needed

### Phase 3: Bytecode Generation

**File**: `src/bytecode.cpp`

#### 3.1 Boxing Methods
Add helper method:
```cpp
void ByteCode::GenerateBoxing(TypeSymbol* primitive_type)
{
    // Load value is already on stack
    TypeSymbol* wrapper_type = BoxingUtility::GetWrapperType(primitive_type);
    MethodSymbol* valueOf = FindValueOfMethod(wrapper_type, primitive_type);

    // Call WrapperClass.valueOf(primitive)
    PutOp(OP_INVOKESTATIC);
    PutU2(RegisterMethodref(wrapper_type, valueOf));

    // Stack effect: primitive → wrapper
}
```

#### 3.2 Unboxing Methods
Add helper method:
```cpp
void ByteCode::GenerateUnboxing(TypeSymbol* wrapper_type)
{
    // Wrapper object is on stack
    TypeSymbol* primitive_type = BoxingUtility::GetPrimitiveType(wrapper_type);
    MethodSymbol* unbox_method = FindUnboxMethod(wrapper_type, primitive_type);

    // Call wrapper.primitiveValue()
    PutOp(OP_INVOKEVIRTUAL);
    PutU2(RegisterMethodref(wrapper_type, unbox_method));

    // Stack effect: wrapper → primitive
}
```

#### 3.3 Integration Points
Modify these methods to call boxing/unboxing:
- `EmitAssignmentExpression()` - Assignment context
- `EmitMethodInvocation()` - Method arguments
- `EmitReturnStatement()` - Return values
- `EmitCastExpression()` - Explicit casts
- `EmitBinaryExpression()` - Operator contexts

### Phase 4: Edge Cases and Error Handling

#### 4.1 Null Unboxing
```java
Integer i = null;
int j = i;  // NullPointerException at runtime
```
**Implementation**:
- No compile-time error (matches javac)
- Runtime NPE handled by JVM when calling `intValue()` on null

#### 4.2 Ternary Operator
```java
Integer i = condition ? 42 : null;  // Boxing
int j = condition ? Integer.valueOf(1) : 2;  // Unboxing
```
**Implementation**: Unify types in ternary operator to least upper bound

#### 4.3 Compound Assignment
```java
Integer i = 10;
i += 5;  // Unbox, add, box
```
**Implementation**: Insert unbox before operation, box after

## Test Plan

### Test 1: Basic Boxing
```java
public class BasicBoxingTest {
    public static void main(String[] args) {
        Integer i = 42;  // int → Integer
        assert i.intValue() == 42;
    }
}
```

### Test 2: Basic Unboxing
```java
public class BasicUnboxingTest {
    public static void main(String[] args) {
        Integer i = Integer.valueOf(42);
        int j = i;  // Integer → int
        assert j == 42;
    }
}
```

### Test 3: Method Arguments
```java
public class MethodBoxingTest {
    static void takesInteger(Integer i) { }
    static void takesInt(int i) { }

    public static void main(String[] args) {
        takesInteger(42);  // Boxing
        takesInt(Integer.valueOf(42));  // Unboxing
    }
}
```

### Test 4: Arithmetic Operations
```java
public class ArithmeticUnboxingTest {
    public static void main(String[] args) {
        Integer a = 10, b = 20;
        int sum = a + b;  // Unbox both
        assert sum == 30;
    }
}
```

### Test 5: Generics Integration
```java
import java.util.ArrayList;
import java.util.List;

public class GenericsBoxingTest {
    public static void main(String[] args) {
        List<Integer> list = new ArrayList<Integer>();
        list.add(42);  // Boxing
        int x = list.get(0);  // Unboxing
        assert x == 42;
    }
}
```

### Test 6: Null Handling
```java
public class NullUnboxingTest {
    public static void main(String[] args) {
        Integer i = null;
        try {
            int j = i;  // Should throw NPE
            assert false;
        } catch (NullPointerException e) {
            // Expected
        }
    }
}
```

### Test 7: Boolean Boxing (Special Case)
```java
public class BooleanBoxingTest {
    public static void main(String[] args) {
        Boolean b = true;
        if (b) {  // Unboxing in condition
            // Expected
        } else {
            assert false;
        }
    }
}
```

### Test 8: Overload Resolution
```java
public class OverloadBoxingTest {
    static String foo(int x) { return "int"; }
    static String foo(Integer x) { return "Integer"; }

    public static void main(String[] args) {
        assert foo(42).equals("int");  // Prefer exact match
        assert foo(Integer.valueOf(42)).equals("Integer");
    }
}
```

## Implementation Order

1. **Week 1**: Phase 1 & 2
   - Create boxing utility class
   - Implement type mapping
   - Update type compatibility checking
   - Basic semantic analysis integration

2. **Week 2**: Phase 3
   - Implement bytecode generation for boxing
   - Implement bytecode generation for unboxing
   - Integrate with assignment expressions
   - Integrate with method calls

3. **Week 3**: Phase 4 & Testing
   - Handle edge cases
   - Method overload resolution with boxing
   - Comprehensive test suite (8+ tests)
   - Integration testing with existing features

## Success Criteria

- ✅ All 8 test cases pass
- ✅ Boxing/unboxing works with generics
- ✅ Method overload resolution considers boxing
- ✅ Arithmetic operators handle wrapper types
- ✅ Null unboxing throws NPE at runtime
- ✅ No regressions in existing tests (12 tests still pass)

## Files to Modify

- `src/control.h` - Add wrapper type references
- `src/control.cpp` - Initialize wrapper types
- `src/semantic.h` - Add boxing utility methods
- `src/semantic.cpp` - Type compatibility and method resolution
- `src/bytecode.h` - Add boxing/unboxing methods
- `src/bytecode.cpp` - Bytecode generation
- `src/expr.cpp` - Expression handling with boxing
- `test-generics/` - New test files

## Risks and Mitigations

**Risk**: Complex interaction with method overloading
**Mitigation**: Implement in phases, add tests incrementally

**Risk**: Performance impact of boxing/unboxing
**Mitigation**: Use valueOf() which caches common values (as per spec)

**Risk**: Breaking existing code
**Mitigation**: Extensive regression testing, ensure all 12 existing tests still pass

## References

- JLS §5.1.7 - Boxing Conversion
- JLS §5.1.8 - Unboxing Conversion
- JLS §15.28 - Constant Expressions (boxing affects constants)
- JSR 201 - Specification

---
*Next Feature After Completion*: Static Imports (complete remaining 75%)
