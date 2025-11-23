# Field Access Type Substitution - Investigation Summary

**Date**: 2025-11-23
**Status**: In Progress - Solution Identified

## Problem

Field access type substitution does not work. Example:
```java
Box<String> stringBox = new Box<String>();
String s = stringBox.value;  // ERROR: returns Object, not String
```

## Key Discoveries

### 1. ProcessFieldAccess Is Not Used for Regular Field Access

Field access expressions like `stringBox.value` are **NOT** processed through `ProcessFieldAccess()`. Instead, they go through **NAME resolution**.

### 2. Field Access Path

The code path is:
1. `stringBox.value` is parsed as a qualified NAME
2. NAME resolution code (around line 2941 in expr.cpp) calls `FindVariableMember(type, name)`
3. This resolves the field and sets `name->symbol` to the VariableSymbol
4. **Type substitution needs to happen AFTER step 3**

### 3. Parameter ParameterizedType Support Added

Added code to copy `parameterized_type` from formal parameter types to parameter VariableSymbols in `ProcessFormalParameters()` (decl.cpp:4090-4095).

## Files Modified

1. **src/symbol.h** - Added `field_declaration` pointer to VariableSymbol
2. **src/decl.cpp** - Set `field_declaration` on field variables, copy `parameterized_type` for parameters
3. **src/expr.cpp** - Added type substitution logic in ProcessFieldAccess (but wrong location)

## Solution

Type substitution for field access needs to be added in the NAME resolution code path, NOT in ProcessFieldAccess. The logic should be added after the call to `FindVariableMember(type, name)` around line 2941 in expr.cpp.

The substitution logic should:
1. Check if the qualified name's base has a parameterized type
2. Check if the field's type is a type parameter
3. Match by name and substitute with corresponding type argument
4. Set `name->resolved_type` to the substituted type

## Status

- ✅ Method return type substitution works
- ✅ ParameterizedType tracking infrastructure complete
- ✅ Parameter parameterized_type support added
- ❌ Field access type substitution - location identified, implementation needed

## Next Steps

1. Remove debug code from expr.cpp
2. Find the exact location in NAME resolution where field access is finalized
3. Add type substitution logic similar to method calls
4. Test with both local variables and parameters
5. Run all tests to verify

## Time Estimate

1-2 hours to complete field access type substitution once the correct location is identified.
