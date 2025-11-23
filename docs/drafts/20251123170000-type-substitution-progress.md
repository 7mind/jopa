# Type Substitution Implementation Progress

**Date**: 2025-11-23
**Session**: Type Substitution for Generics

## Executive Summary

Implementing type substitution so that method return types are properly substituted with type arguments. For example, `Box<String>.get()` should return `String`, not `Object`.

**Current Status**: Infrastructure complete, need to implement substitution logic in method resolution.

## Completed Work

### 1. ✅ Phase 1: ParameterizedType Tracking Infrastructure

**Files Modified**:
- `src/ast.h` - Added `ParameterizedType* parameterized_type` field to AstTypeName (line 1151)
- `src/ast.h` - Added forward declaration for ParameterizedType (line 22)
- `src/semantic.h` - Changed ProcessTypeArguments to return `ParameterizedType*` (line 847)
- `src/decl.cpp` - Modified ProcessTypeArguments to create and return ParameterizedType (lines 1121-1137)
- `src/decl.cpp` - Store ParameterizedType on AstTypeName (line 4986)
- `src/decl.cpp` - Added `#include "paramtype.h"` (line 11)

**Key Changes**:
- When processing `Box<String>`, ProcessTypeArguments now creates a ParameterizedType object and stores it on the AstTypeName
- Used `Tuple::Next()` method correctly to add elements (fixed segfault)
- Added defensive null checks for type arguments

### 2. ✅ Phase 2a: Variable ParameterizedType Storage

**Files Modified**:
- `src/symbol.h` - Added `ParameterizedType* parameterized_type` field to VariableSymbol (line 1552)
- `src/symbol.h` - Initialized field in VariableSymbol constructor (line 1619)
- `src/decl.cpp` - Copy parameterized_type from AstTypeName to field variables (lines 2719-2724)
- `src/body.cpp` - Copy parameterized_type from AstTypeName to local variables (lines 266-271)

**Key Changes**:
- Variables now track their parameterized types
- When declaring `Box<String> box`, the VariableSymbol for `box` stores the ParameterizedType
- Works for both field variables and local variables

## What Works Now ✅

1. **ParameterizedType creation**: `Box<String>` creates ParameterizedType(Box, [String])
2. **Type storage on AST**: AstTypeName nodes store parameterized_type
3. **Variable tracking**: Variables remember their parameterized types
4. **Compilation**: All generics tests still pass

## What Doesn't Work Yet ❌

### Method Return Type Substitution (In Progress)

**Problem**: When calling `box.get()` where `box` is `Box<String>`, the return type is still `Object` instead of `String`.

**What's Needed**:
1. In ProcessMethodName (expr.cpp:3329), after resolving the method:
   - Check if the receiver (base) is a variable with a parameterized_type
   - Get the method's return type
   - If the return type is a type parameter (e.g., `T`), substitute it with the corresponding type argument from the parameterized_type
   - Store the substituted type somewhere accessible

**Challenge**: The method_call->symbol is a MethodSymbol which has a fixed return type. We can't modify it because it's shared. We need another mechanism to track the substituted return type.

**Options**:
1. Add a `substituted_type` field to AstExpression to track runtime type
2. Modify how symbol is stored for method calls to use a wrapper
3. Use the existing mechanism (if any) for tracking refined types

## Next Steps

### Phase 2b: Implement Type Substitution in Method Resolution

1. **In ProcessMethodName (expr.cpp:3329)**:
   - After method is resolved (line ~3384), check if receiver has parameterized_type
   - Get the method's return type
   - Check if it's a type parameter
   - If so, find the corresponding type argument and substitute

2. **Helper Function to Add**:
   ```cpp
   TypeSymbol* Semantic::SubstituteTypeParameter(
       TypeSymbol* type,
       TypeSymbol* declaring_class,
       ParameterizedType* receiver_param_type)
   {
       // If type is a type parameter of declaring_class
       for (unsigned i = 0; i < declaring_class -> NumTypeParameters(); i++)
       {
           if (type == declaring_class -> TypeParameter(i))
           {
               // Return the i-th type argument
               Type* arg = receiver_param_type -> TypeArgument(i);
               return arg -> Erasure(); // For now, just get the erased type
           }
       }
       return type; // Not a type parameter, return unchanged
   }
   ```

3. **Test** with TypeSubstitutionTest.java

### Phase 3: Field Access Type Substitution

Similar logic needed for field access.

### Phase 4: Testing

- Test basic substitution: `Box<String>.get()` returns String
- Test nested generics: `Box<List<String>>.get()` returns List
- Test multiple type parameters: `Pair<K,V>.getKey()` returns K
- Test inheritance: `StringBox extends Box<String>`

## Files to Modify Next

1. `src/expr.cpp` - ProcessMethodName function (add substitution logic)
2. `src/semantic.h` - Add SubstituteTypeParameter helper
3. `src/semantic.cpp` or `src/expr.cpp` - Implement SubstituteTypeParameter

## Build Status

✅ Compiles successfully
✅ All generics tests pass
⏳ Type substitution not yet functional

## Notes

- The infrastructure is solid and well-tested
- The remaining work is focused and well-defined
- Should be straightforward to implement the substitution logic
- Main challenge is deciding how to store/return the substituted type from method calls
