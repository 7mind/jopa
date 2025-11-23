# Java 7 Implementation Roadmap
## JOPA Compiler - Comprehensive Implementation Plan

**Date:** 2025-01-23
**Status:** Planning Phase
**Target:** Complete Java 7 (JSR 336) Language and Bytecode Support

---

## Executive Summary

This document provides a comprehensive roadmap for implementing Java 7 support in the JOPA compiler. Java 7 introduces significant language enhancements (JSR 334 - Project Coin), revolutionary JVM changes (JSR 292 - invokedynamic), bytecode format updates (version 51.0), and new I/O APIs (JSR 203 - NIO.2).

**Current State:**
- ✅ Java 5 features: 100% complete (generics, enums, varargs, foreach, autoboxing, static imports, annotations)
- ✅ Java 6 bytecode: Partial support (version 50.0, basic reflection)

**Complexity Assessment:**
- **High Complexity:** invokedynamic, Method Handles, StackMapTable generation
- **Medium Complexity:** Try-with-resources, multi-catch, precise rethrow, strings in switch
- **Low Complexity:** Binary literals, underscores in literals, diamond operator, SafeVarargs

---

## Table of Contents

1. [JSR Overview](#jsr-overview)
2. [Phase 1: Language Features (JSR 334 - Project Coin)](#phase-1-language-features)
3. [Phase 2: Bytecode Version 51.0](#phase-2-bytecode-version-51)
4. [Phase 3: invokedynamic and Method Handles (JSR 292)](#phase-3-invokedynamic-and-method-handles)
5. [Phase 4: Runtime Library Updates](#phase-4-runtime-library-updates)
6. [Phase 5: Testing and Validation](#phase-5-testing-and-validation)
7. [Implementation Dependencies](#implementation-dependencies)
8. [Risk Assessment](#risk-assessment)

---

## JSR Overview

### Java SE 7 Component JSRs

1. **JSR 336:** Java SE 7 Release Contents (umbrella specification)
2. **JSR 334:** Small Enhancements to the Java Programming Language (Project Coin)
3. **JSR 292:** Supporting Dynamically Typed Languages on the Java Platform
4. **JSR 203:** More New I/O APIs for the Java Platform (NIO.2)
5. **JSR 166y:** Concurrency and Collections Updates (Fork/Join Framework)

**Compiler-Relevant JSRs:** JSR 334, JSR 292, bytecode version 51.0 changes
**Runtime-Only JSRs:** JSR 203, JSR 166y (library additions, minimal compiler impact)

---

## Phase 1: Language Features (JSR 334 - Project Coin)

### 1.1 Binary Literals and Underscores in Numeric Literals

**Complexity:** LOW
**Estimated Effort:** 2-3 days
**Dependencies:** None

#### Description
- Binary literals: `0b1010_1100` or `0B1111_0000`
- Underscores as digit separators: `1_000_000`, `0xFF_EC_DE_5E`, `3.14_15_92`
- Valid in binary, octal, decimal, hexadecimal, and floating-point literals
- Underscores cannot appear at start/end or adjacent to decimal point

#### Implementation Tasks

1. **Scanner Changes** (`src/scanner.cpp/h`)
   - Extend `ScanNumber()` to recognize `0b` and `0B` prefixes
   - Add binary digit scanning loop (`0` and `1` only)
   - Modify all numeric scanning functions to accept and skip underscores
   - Validate underscore placement rules:
     - Not at beginning or end of number
     - Not before or after `.` in floating-point
     - Not before `L`, `F`, `D` suffixes
     - Not adjacent to `0x`, `0X`, `0b`, `0B` prefixes

2. **Token Representation**
   - Add `TK_IntegerLiteral_Binary` token type (or reuse with flag)
   - Strip underscores before integer/float conversion in `LiteralValue` class

3. **Semantic Analysis** (`src/expr.cpp`)
   - Binary literals convert to int/long like octal/hex
   - No semantic changes needed (underscores are syntactic sugar)

4. **Error Reporting** (`src/error.cpp/h`)
   - Add errors for invalid underscore placement
   - Add errors for invalid binary digits (2-9, A-F)

5. **Testing**
   - Binary literal compilation tests (`0b0000` to `0b1111_1111_1111_1111`)
   - Underscore separator tests (all numeric types, all bases)
   - Error tests for invalid placements
   - Runtime tests for correct values

**Files to Modify:**
- `src/scanner.cpp/h` - Primary changes
- `src/javaact.cpp` - Possibly minimal
- `src/error.cpp/h` - New error messages
- `test/java7/binary_literals.java` (new)
- `test/java7/underscore_literals.java` (new)

---

### 1.2 Strings in Switch Statements

**Complexity:** MEDIUM
**Estimated Effort:** 5-7 days
**Dependencies:** None

#### Description
- Allow `String` type in switch expressions
- Case labels accept string literals
- Desugaring strategy: two-level switch (hash code, then equals)
- Null handling: throw `NullPointerException` before switch

#### Implementation Tasks

1. **Parser/AST** (`src/parser.cpp`, `src/ast.h`)
   - Modify switch statement parsing to accept String expressions
   - Parse string literals in case labels
   - Existing `AstSwitchStatement` should handle this with type system

2. **Semantic Analysis** (`src/body.cpp`)
   - Relax switch expression type check to allow `String`
   - Validate case labels are string literals (compile-time constants)
   - Check for duplicate case values
   - Verify all case strings are unique (including hash collisions)

3. **Desugaring Strategy** (`src/bytecode.cpp`)

   Transform:
   ```java
   switch (str) {
       case "hello": ...
       case "world": ...
       default: ...
   }
   ```

   Into equivalent:
   ```java
   String s = str;
   int hash = (s == null) ? 0 : s.hashCode();
   switch (hash) {
       case 99162322: // "hello".hashCode()
           if (s.equals("hello")) { ... }
           break;
       case 113318802: // "world".hashCode()
           if (s.equals("world")) { ... }
           break;
       default: ...
   }
   ```

4. **Bytecode Generation** (`src/bytecode.cpp`)
   - Generate two-level switch structure:
     - Level 1: tableswitch/lookupswitch on hashCode()
     - Level 2: if statements with String.equals() calls
   - Handle null check before hashCode() call
   - Optimize: use tableswitch if hash codes are dense, lookupswitch otherwise
   - Preserve case label line numbers for debugging

5. **Constant Pool Management**
   - Add CONSTANT_String entries for each case literal
   - Reference `java/lang/String.hashCode()I` method
   - Reference `java/lang/String.equals(Ljava/lang/Object;)Z` method

6. **Error Reporting**
   - Error for non-constant string case labels
   - Error for duplicate case strings

7. **Testing**
   - Basic string switch (2-3 cases)
   - Large string switch (10+ cases) to test hash collisions
   - Null input test (should throw NPE)
   - Empty string case
   - Unicode string cases
   - Fall-through behavior

**Files to Modify:**
- `src/body.cpp` - Semantic analysis
- `src/bytecode.cpp` - Desugaring and code generation
- `src/error.cpp/h` - Error messages
- `test/java7/switch_string_basic.java` (new)
- `test/java7/switch_string_advanced.java` (new)

**Reference:** JLS §14.11 (The switch Statement), JVMS §3.10

---

### 1.3 Diamond Operator (Improved Type Inference)

**Complexity:** MEDIUM-HIGH
**Estimated Effort:** 7-10 days
**Dependencies:** Existing generics implementation

#### Description
- Allow `<>` in generic instance creation when types can be inferred
- Example: `Map<String, List<Integer>> m = new HashMap<>();`
- Compiler infers `<String, List<Integer>>` from left-hand side
- Limited to constructor invocations (not method calls yet)

#### Implementation Tasks

1. **Parser/AST** (`src/parser.cpp`, `src/ast.h`)
   - Recognize `<>` in class instance creation expressions
   - Add flag or representation for "inferred type arguments"
   - Extend `AstClassCreationExpression` to mark diamond usage

2. **Semantic Analysis** (`src/expr.cpp`)
   - Detect `<>` in constructor invocation
   - Infer type arguments from target type (assignment, method argument, return)
   - Algorithm:
     1. Identify target type from context (left side of assignment, parameter type, etc.)
     2. Extract type arguments from target type
     3. Substitute into constructor declaration
     4. Validate constructor signature matches
   - Handle complex cases:
     - Nested generics: `List<List<String>> = new ArrayList<>()`
     - Bounded types: ensure inferred types satisfy bounds
     - Anonymous classes: diamond NOT allowed (error)

3. **Type Inference Engine**
   - Leverage existing generic type system (`src/typeparam.cpp/h`, `src/paramtype.cpp/h`)
   - Implement target typing: propagate expected type down to expression
   - Handle method chaining and nested expressions

4. **Error Reporting**
   - Error when `<>` used with anonymous class
   - Error when type cannot be inferred (no target type available)
   - Error when inferred type doesn't satisfy bounds

5. **Testing**
   - Basic diamond: `List<String> list = new ArrayList<>()`
   - Nested generics: `Map<String, List<Integer>> map = new HashMap<>()`
   - Constructor arguments: inferred types with arguments
   - Method invocation context: `foo(new ArrayList<>())`
   - Error cases: anonymous classes, no target type

**Files to Modify:**
- `src/parser.cpp` - Parse `<>`
- `src/ast.h` - Mark diamond usage
- `src/expr.cpp` - Type inference implementation
- `src/typeparam.cpp/h` - Type inference utilities
- `src/error.cpp/h` - Error messages
- `test/java7/diamond_operator.java` (new)

**Reference:** JLS §15.9 (Class Instance Creation Expressions), specifically §15.9.1

---

### 1.4 Multi-Catch Exception Handling

**Complexity:** MEDIUM
**Estimated Effort:** 5-7 days
**Dependencies:** None

#### Description
- Catch multiple exception types in single catch block
- Syntax: `catch (IOException | SQLException ex)`
- Exception parameter is implicitly final
- Bytecode: generate separate exception table entries for each type

#### Implementation Tasks

1. **Parser/AST** (`src/parser.cpp`, `src/ast.h`)
   - Extend catch clause parsing to accept `|`-separated types
   - Modify `AstCatchClause` to hold multiple exception types
   - Store list of `AstType*` instead of single type

2. **Semantic Analysis** (`src/body.cpp`)
   - Validate each exception type in multi-catch:
     - All must be subclasses of `Throwable`
     - No type can be subclass of another in same catch (redundancy error)
   - Mark exception parameter as implicitly final
   - Type of exception parameter is least upper bound (LUB) of all caught types
     - For bytecode: use first common superclass
     - For type checking: intersection type (conceptually)

3. **Bytecode Generation** (`src/bytecode.cpp`)
   - Generate multiple exception table entries, one per exception type
   - All entries point to same handler PC
   - Handler code is shared for all exception types
   - Exception parameter type in LocalVariableTable: use most specific common supertype

4. **Exception Table Structure**
   ```
   Exception Table:
     from    to  target type
         0    10    13   Class java/io/IOException
         0    10    13   Class java/sql/SQLException
   ```

5. **Definite Assignment Analysis**
   - Exception parameter is implicitly final (cannot be reassigned)
   - Enforce this in semantic analysis

6. **Error Reporting**
   - Error for redundant exception types in multi-catch
   - Error if attempting to assign to exception parameter
   - Error if exception types are not disjoint

7. **Testing**
   - Basic multi-catch (2 unrelated exception types)
   - Multi-catch with 3+ types
   - Verify exception parameter is final (assignment should error)
   - Error test for redundant types (`IOException | FileNotFoundException`)
   - Runtime test: verify correct exception caught

**Files to Modify:**
- `src/parser.cpp` - Parse `|` in catch clause
- `src/ast.h` - Extend `AstCatchClause`
- `src/body.cpp` - Semantic analysis and finality enforcement
- `src/bytecode.cpp` - Multiple exception table entries
- `src/error.cpp/h` - Error messages
- `test/java7/multi_catch.java` (new)

**Reference:** JLS §14.20 (The try statement)

---

### 1.5 More Precise Rethrow

**Complexity:** MEDIUM
**Estimated Effort:** 4-5 days
**Dependencies:** Multi-catch implementation

#### Description
- Compiler analyzes try block to determine actual exception types thrown
- Allows more precise `throws` clause when rethrowing
- Exception parameter in catch is implicitly final
- Requires flow analysis of try block

#### Implementation Tasks

1. **Semantic Analysis** (`src/body.cpp`)
   - Track all exception types thrown in try block
   - When analyzing catch-and-rethrow pattern:
     ```java
     void foo() throws IOException, SQLException {
         try { ... }
         catch (Exception ex) {
             log(ex);
             throw ex; // Compiler knows only IOException, SQLException possible
         }
     }
     ```
   - Determine precise set of exception types that can be rethrown
   - Validate method's `throws` clause includes these precise types

2. **Exception Flow Analysis**
   - Enhance existing exception analysis in semantic phase
   - For each `throw` statement in try block, record exception type
   - Compute union of all exception types
   - Intersect with catch clause's caught types
   - Result: precise set of exceptions that can be rethrown

3. **Final Exception Parameter**
   - Exception parameter in catch is implicitly final (even without `final` keyword)
   - Detect reassignment and produce error
   - If reassigned, fall back to declared catch type (disable precise rethrow)

4. **Bytecode Generation**
   - No bytecode changes needed (analysis is compile-time only)
   - Signature attributes unaffected

5. **Error Reporting**
   - Error if method's `throws` clause doesn't cover rethrown exceptions
   - Warning if exception parameter reassigned (disables precise rethrow)

6. **Testing**
   - Basic precise rethrow: catch `Exception`, rethrow, declare specific types
   - Multi-catch with rethrow
   - Test that reassignment disables precise rethrow
   - Nested try-catch with rethrow

**Files to Modify:**
- `src/body.cpp` - Exception flow analysis
- `src/semantic.h` - Exception tracking structures
- `src/error.cpp/h` - Error messages
- `test/java7/precise_rethrow.java` (new)

**Reference:** JLS §11.2.2 (Exception Analysis of Statements)

---

### 1.6 Try-with-Resources

**Complexity:** HIGH
**Estimated Effort:** 10-14 days
**Dependencies:** None (but requires AutoCloseable runtime class)

#### Description
- Automatically close resources implementing `AutoCloseable` or `Closeable`
- Syntax: `try (ResourceType res = new Resource()) { ... }`
- Resources closed in reverse order of creation
- Suppressed exceptions tracked via `Throwable.addSuppressed()`
- Complex desugaring with multiple exception paths

#### Implementation Tasks

1. **Parser/AST** (`src/parser.cpp`, `src/ast.h`)
   - Extend try statement parsing to accept resource specifications
   - Add `AstResourceDeclaration` node type (similar to local variable)
   - Modify `AstTryStatement` to hold list of resources
   - Resources are `(Type id = Expression; ...)` separated by semicolons

2. **Semantic Analysis** (`src/body.cpp`)
   - Validate each resource type implements `AutoCloseable` or `Closeable`
   - Resources are implicitly final
   - Resources are in scope only within try block (not catch/finally)
   - Process resource initializers as local variable declarations
   - Check `close()` method exists and is accessible

3. **Desugaring Strategy** (`src/bytecode.cpp`)

   Transform:
   ```java
   try (FileInputStream fis = new FileInputStream("file.txt")) {
       // use fis
   }
   ```

   Into equivalent (simplified):
   ```java
   FileInputStream fis = new FileInputStream("file.txt");
   Throwable primaryException = null;
   try {
       // use fis
   } catch (Throwable t) {
       primaryException = t;
       throw t;
   } finally {
       if (fis != null) {
           if (primaryException != null) {
               try {
                   fis.close();
               } catch (Throwable closeException) {
                   primaryException.addSuppressed(closeException);
               }
           } else {
               fis.close();
           }
       }
   }
   ```

   For multiple resources:
   ```java
   try (R1 r1 = new R1(); R2 r2 = new R2()) { ... }
   ```

   Close in reverse order: r2.close(), then r1.close()
   Each close wrapped in try-catch for addSuppressed()

4. **Bytecode Generation** (`src/bytecode.cpp`)
   - Generate complex control flow:
     - Resource initialization code
     - Try block with exception handler
     - Finally block with close logic
     - Suppressed exception handling
   - Call `Throwable.addSuppressed(Throwable)` method
   - Handle null resources (skip close if null)
   - Maintain proper exception table entries

5. **Constant Pool Management**
   - Reference `java/lang/AutoCloseable.close()V` or `java/io/Closeable.close()V`
   - Reference `java/lang/Throwable.addSuppressed(Ljava/lang/Throwable;)V`
   - Reference resource classes and constructors

6. **Variable Scope and Finality**
   - Resources are final (cannot be reassigned)
   - Resources only visible in try block
   - Local variable table entries for resources

7. **Error Reporting**
   - Error if resource type doesn't implement AutoCloseable
   - Error if attempting to reassign resource variable
   - Error if resource used outside try block

8. **Testing**
   - Single resource try-with-resources
   - Multiple resources (test reverse close order)
   - Exception in try block + exception in close() (test addSuppressed)
   - Null resource (should not call close)
   - Resource with catch and finally blocks
   - Nested try-with-resources

**Files to Modify:**
- `src/parser.cpp` - Parse resource specifications
- `src/ast.h` - Add `AstResourceDeclaration`, extend `AstTryStatement`
- `src/body.cpp` - Semantic analysis, scope management
- `src/bytecode.cpp` - Complex desugaring and code generation
- `src/error.cpp/h` - Error messages
- `runtime/java/lang/AutoCloseable.java` (new)
- `runtime/java/lang/Throwable.java` - Add `addSuppressed()` method
- `test/java7/try_with_resources_basic.java` (new)
- `test/java7/try_with_resources_multiple.java` (new)
- `test/java7/try_with_resources_exceptions.java` (new)

**Reference:** JLS §14.20.3 (try-with-resources)

---

### 1.7 SafeVarargs Annotation

**Complexity:** LOW
**Estimated Effort:** 2-3 days
**Dependencies:** Existing annotation support

#### Description
- Suppress heap pollution warnings from varargs with non-reifiable types
- Applicable to constructors and final/static methods only (private in Java 9+)
- Annotation type: `@SafeVarargs`
- Runtime retention (like @Override, @Deprecated)

#### Implementation Tasks

1. **Annotation Declaration** (`runtime/java/lang/SafeVarargs.java`)
   - Create annotation interface
   - `@Target({CONSTRUCTOR, METHOD})`
   - `@Retention(RUNTIME)`

2. **Semantic Analysis** (`src/decl.cpp`)
   - Validate @SafeVarargs only on:
     - Constructors
     - Final methods
     - Static methods
   - Error if applied to non-varargs method
   - Suppress unchecked warnings for varargs when present

3. **Warning Suppression**
   - Identify heap pollution warnings for varargs with generics
   - Suppress if method annotated with @SafeVarargs
   - Move warning from call site to declaration (Java 7 behavior)

4. **Bytecode Generation**
   - Emit RuntimeVisibleAnnotations attribute with @SafeVarargs

5. **Error Reporting**
   - Error if @SafeVarargs on non-final, non-static method
   - Error if @SafeVarargs on non-varargs method

6. **Testing**
   - @SafeVarargs on constructor
   - @SafeVarargs on static method
   - @SafeVarargs on final method
   - Error tests for invalid usage

**Files to Modify:**
- `src/decl.cpp` - Validation and warning suppression
- `src/error.cpp/h` - Error messages
- `runtime/java/lang/SafeVarargs.java` (new)
- `test/java7/safevarargs.java` (new)

**Reference:** JLS §9.6.4.7 (@SafeVarargs)

---

## Phase 2: Bytecode Version 51.0

### 2.1 Class File Version Update

**Complexity:** LOW
**Estimated Effort:** 1-2 days
**Dependencies:** None

#### Description
- Update class file major version to 51 (0x0033)
- Minor version remains 0
- Enable `-target 1.7` compiler flag

#### Implementation Tasks

1. **Version Constants** (`src/class.h`, `src/bytecode.cpp`)
   - Add constant for Java 7 bytecode version: `JAVA_7_VERSION = 51`
   - Update `JikesOption` to support SDK1_7 release level

2. **Compiler Options** (`src/option.cpp/h`)
   - Add `-target 1.7` option
   - Add `-source 1.7` option
   - Set default to 1.7 when `-DJIKES_ENABLE_SOURCE_17=ON`

3. **Bytecode Generation** (`src/bytecode.cpp`)
   - Write major version 51 when target is 1.7
   - Ensure all class files get correct version

4. **Testing**
   - Compile with `-target 1.7`, verify class file version
   - Load with Java 7+ JVM, verify execution
   - Verify Java 6 JVM rejects version 51 class files

**Files to Modify:**
- `src/class.h` - Version constant
- `src/option.cpp/h` - Command-line flags
- `src/bytecode.cpp` - Version writing
- `CMakeLists.txt` - Add JIKES_ENABLE_SOURCE_17 option

---

### 2.2 StackMapTable Attribute (Mandatory)

**Complexity:** VERY HIGH
**Estimated Effort:** 20-30 days
**Dependencies:** Complete bytecode generation implementation

#### Description
- StackMapTable required for all methods in Java 7 (was optional in Java 6)
- Type verification by JVM uses stack maps instead of type inference
- Specifies verification types for locals and stack at branch targets
- Complex data structure with multiple frame types

#### Background

In Java 6 and earlier, the JVM performed verification by type inference (dataflow analysis). Java 7 requires explicit StackMapTable attributes, making verification faster and simpler for the JVM but more complex for the compiler.

#### Implementation Tasks

1. **Frame Types** (define in `src/stackmap.h` - new file)
   - `same_frame`: stack is empty, locals unchanged
   - `same_locals_1_stack_item_frame`: one stack item
   - `same_locals_1_stack_item_frame_extended`: extended version
   - `chop_frame`: remove locals
   - `same_frame_extended`: extended same
   - `append_frame`: add locals
   - `full_frame`: complete specification

2. **Verification Type Info**
   - `Top_variable_info`: top of stack (unusable)
   - `Integer_variable_info`: int, boolean, byte, char, short
   - `Float_variable_info`: float
   - `Double_variable_info`: double (occupies 2 slots)
   - `Long_variable_info`: long (occupies 2 slots)
   - `Null_variable_info`: null reference
   - `UninitializedThis_variable_info`: uninitialized `this`
   - `Object_variable_info`: class type
   - `Uninitialized_variable_info`: uninitialized object (new but before constructor)

3. **Stack Map Generator** (`src/stackmap.cpp` - new file)
   - Analyze bytecode control flow graph
   - Identify all branch targets and exception handlers
   - Compute local variable types at each target
   - Compute operand stack types at each target
   - Generate minimal frame representations (compression)

4. **Control Flow Analysis**
   - Build CFG from bytecode instructions
   - Track type state through all paths
   - Handle exception paths (exception on stack)
   - Detect unreachable code

5. **Type Tracking**
   - Track local variable types through bytecode
   - Track operand stack types through bytecode
   - Handle wide types (long, double occupy 2 slots)
   - Handle uninitialized objects (from new instruction to constructor call)

6. **Frame Compression**
   - Choose minimal frame type representation
   - Use offset deltas for sequential frames
   - Prefer smaller frame types (same, append, chop) over full

7. **Integration with Bytecode Generator** (`src/bytecode.cpp`)
   - During code generation, record branch targets
   - After code generation, invoke stack map generator
   - Emit StackMapTable attribute after exception table

8. **Attribute Structure**
   ```
   StackMapTable_attribute {
       u2 attribute_name_index;
       u4 attribute_length;
       u2 number_of_entries;
       stack_map_frame entries[number_of_entries];
   }
   ```

9. **Testing Strategy**
   - Simple method with no branches: minimal stack map
   - Method with if statements
   - Method with loops
   - Method with try-catch blocks
   - Method with switch statements
   - Method with uninitialized objects (constructor calls)
   - Complex method with all control flow types
   - Verify stack maps with `javap -v`
   - Verify JVM accepts generated class files

10. **Error Handling**
    - Internal compiler error if stack map generation fails
    - Should never happen for valid bytecode, so assert/fail fast

**Files to Create:**
- `src/stackmap.h` - StackMap data structures
- `src/stackmap.cpp` - StackMap generation algorithm

**Files to Modify:**
- `src/bytecode.cpp` - Integration, attribute emission
- `src/code.h` - Add StackMapTable data structures
- `CMakeLists.txt` - Add new source files

**Testing:**
- `test/java7/stackmap_simple.java` (new)
- `test/java7/stackmap_branches.java` (new)
- `test/java7/stackmap_exceptions.java` (new)
- `test/java7/stackmap_loops.java` (new)

**Reference:** JVMS §4.7.4 (StackMapTable Attribute), JVMS §4.10.1 (Verification by Type Checking)

**Notes:**
- This is the most complex part of Java 7 implementation
- Consider incremental approach: start with simple methods, add complexity
- Extensive testing required with real JVM verification
- May need debugging support: dump generated stack maps

---

## Phase 3: invokedynamic and Method Handles (JSR 292)

### 3.1 invokedynamic Bytecode Instruction

**Complexity:** VERY HIGH
**Estimated Effort:** 15-20 days
**Dependencies:** Bytecode version 51.0, new constant pool entries

#### Description
- New bytecode instruction: `invokedynamic` (opcode 186, 0xBA)
- Supports dynamic method invocation for dynamic languages
- For Java 7: NOT directly usable from Java source (only from bytecode)
- Java 8 uses it for lambda expressions

#### Java 7 Scope Decision

**RECOMMENDATION:** Defer invokedynamic implementation until after Java 7 core features are complete.

**Rationale:**
1. Java 7 source code does NOT generate invokedynamic instructions
2. Only dynamic languages (JRuby, Groovy, Jython) use it directly
3. Java uses it starting in Java 8 for lambdas
4. Implementing invokedynamic requires:
   - New constant pool entries
   - BootstrapMethods attribute
   - Complex runtime linkage semantics
5. JOPA compiler's primary goal: compile Java source, not support dynamic languages

**If Implemented Later:**

1. **New Constant Pool Entries**
   - `CONSTANT_MethodHandle` (tag 15)
   - `CONSTANT_MethodType` (tag 16)
   - `CONSTANT_InvokeDynamic` (tag 18)

2. **BootstrapMethods Attribute**
   - Class-level attribute
   - Lists bootstrap methods for invokedynamic sites
   - Structure:
     ```
     BootstrapMethods_attribute {
         u2 attribute_name_index;
         u4 attribute_length;
         u2 num_bootstrap_methods;
         bootstrap_method bootstrap_methods[num_bootstrap_methods];
     }
     ```

3. **Bytecode Emission**
   - `invokedynamic` instruction (0xBA)
   - 2-byte constant pool index (CONSTANT_InvokeDynamic)
   - 2 bytes of zero padding (reserved)

**Alternative: Minimal Implementation**
- Add constant pool entry types (for class file reading compatibility)
- Add BootstrapMethods attribute parsing
- Do NOT generate invokedynamic from source (not needed for Java 7 source)
- Enables reading Java 8+ class files without crashing

**Files to Modify (if implemented):**
- `src/class.h` - Constant pool tag constants
- `src/bytecode.cpp` - Constant pool entry reading/writing
- `src/code.cpp` - invokedynamic instruction emission

**Testing:**
- Read Java 8 class file with invokedynamic (compatibility test)
- Do NOT test source generation (not applicable)

**Reference:** JVMS §4.4.10 (CONSTANT_InvokeDynamic), JVMS §4.7.23 (BootstrapMethods), JVMS §6.5.invokedynamic

---

### 3.2 Method Handles (java.lang.invoke)

**Complexity:** HIGH
**Estimated Effort:** 10-15 days (if implemented)
**Dependencies:** invokedynamic infrastructure

#### Description
- `java.lang.invoke` package (MethodHandle, MethodType, CallSite, etc.)
- Low-level mechanism for method invocation
- Used by dynamic languages and lambda implementation
- For compiler: primarily library work, minimal compiler changes

#### Java 7 Scope Decision

**RECOMMENDATION:** Do NOT implement java.lang.invoke in JOPA runtime.

**Rationale:**
1. Method handles are primarily used by dynamic language implementers
2. Java source code in Java 7 does not directly use method handles
3. Runtime library work, not compiler work
4. JOPA runtime is minimal; full java.lang.invoke would require:
   - MethodHandle class and subclasses
   - MethodType class
   - CallSite and subclasses
   - MethodHandles lookup class
   - Complex native implementation

**If Implemented:**
- Add minimal stubs to runtime library
- Real implementation would require JVM support (native methods)

**Files to Create (if implemented):**
- `runtime/java/lang/invoke/MethodHandle.java`
- `runtime/java/lang/invoke/MethodType.java`
- `runtime/java/lang/invoke/CallSite.java`

**Testing:**
- Compatibility: compile code that imports java.lang.invoke
- Do NOT test actual method handle invocation (requires JVM support)

---

## Phase 4: Runtime Library Updates

### 4.1 AutoCloseable Interface

**Complexity:** LOW
**Estimated Effort:** 1 day
**Dependencies:** None

#### Implementation

Create `runtime/java/lang/AutoCloseable.java`:
```java
package java.lang;

public interface AutoCloseable {
    void close() throws Exception;
}
```

Update `runtime/java/io/Closeable.java` to extend AutoCloseable:
```java
package java.io;

public interface Closeable extends java.lang.AutoCloseable {
    void close() throws java.io.IOException;
}
```

**Files to Create:**
- `runtime/java/lang/AutoCloseable.java`

**Files to Modify:**
- `runtime/java/io/Closeable.java`

---

### 4.2 Throwable.addSuppressed()

**Complexity:** LOW
**Estimated Effort:** 1 day
**Dependencies:** None

#### Implementation

Add to `runtime/java/lang/Throwable.java`:
```java
private Throwable[] suppressedExceptions;

public final void addSuppressed(Throwable exception) {
    if (exception == null)
        throw new NullPointerException();
    if (exception == this)
        throw new IllegalArgumentException("Self-suppression not permitted");

    if (suppressedExceptions == null)
        suppressedExceptions = new Throwable[1];
    else {
        Throwable[] tmp = new Throwable[suppressedExceptions.length + 1];
        System.arraycopy(suppressedExceptions, 0, tmp, 0, suppressedExceptions.length);
        suppressedExceptions = tmp;
    }
    suppressedExceptions[suppressedExceptions.length - 1] = exception;
}

public final Throwable[] getSuppressed() {
    if (suppressedExceptions == null)
        return new Throwable[0];
    Throwable[] result = new Throwable[suppressedExceptions.length];
    System.arraycopy(suppressedExceptions, 0, result, 0, suppressedExceptions.length);
    return result;
}
```

**Files to Modify:**
- `runtime/java/lang/Throwable.java`

---

### 4.3 SafeVarargs Annotation

**Complexity:** LOW
**Estimated Effort:** 1 day
**Dependencies:** None

#### Implementation

Create `runtime/java/lang/SafeVarargs.java`:
```java
package java.lang;

import java.lang.annotation.*;

@Documented
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.CONSTRUCTOR, ElementType.METHOD})
public @interface SafeVarargs {}
```

**Files to Create:**
- `runtime/java/lang/SafeVarargs.java`

---

### 4.4 Objects Utility Class

**Complexity:** LOW
**Estimated Effort:** 1-2 days
**Dependencies:** None

#### Description
Java 7 added `java.util.Objects` with utility methods like `equals()`, `hashCode()`, `toString()`, `requireNonNull()`.

#### Implementation

Create `runtime/java/util/Objects.java`:
```java
package java.util;

public final class Objects {
    private Objects() {}

    public static boolean equals(Object a, Object b) {
        return (a == b) || (a != null && a.equals(b));
    }

    public static int hashCode(Object o) {
        return o != null ? o.hashCode() : 0;
    }

    public static String toString(Object o) {
        return String.valueOf(o);
    }

    public static <T> T requireNonNull(T obj) {
        if (obj == null)
            throw new NullPointerException();
        return obj;
    }

    public static <T> T requireNonNull(T obj, String message) {
        if (obj == null)
            throw new NullPointerException(message);
        return obj;
    }
}
```

**Files to Create:**
- `runtime/java/util/Objects.java`

---

## Phase 5: Testing and Validation

### 5.1 Feature Test Suite

Create comprehensive tests for each Java 7 feature:

**Test Categories:**
1. **Compilation Tests** - Verify successful compilation
2. **Runtime Tests** - Execute with JVM, verify behavior
3. **Error Tests** - Verify appropriate compile errors
4. **Integration Tests** - Combine multiple features

**Test Files to Create:**
- `test/java7/binary_literals.java` - Binary literal syntax
- `test/java7/underscore_literals.java` - Underscore separators
- `test/java7/switch_string_basic.java` - Basic string switch
- `test/java7/switch_string_advanced.java` - Complex string switch (hash collisions, null)
- `test/java7/diamond_operator.java` - Type inference with `<>`
- `test/java7/multi_catch.java` - Multi-catch exception handling
- `test/java7/precise_rethrow.java` - Precise rethrow analysis
- `test/java7/try_with_resources_basic.java` - Single resource
- `test/java7/try_with_resources_multiple.java` - Multiple resources
- `test/java7/try_with_resources_exceptions.java` - Exception suppression
- `test/java7/safevarargs.java` - SafeVarargs annotation
- `test/java7/stackmap_simple.java` - Simple stack map generation
- `test/java7/stackmap_branches.java` - Branches and stack maps
- `test/java7/stackmap_exceptions.java` - Exception handlers and stack maps
- `test/java7/stackmap_loops.java` - Loops and stack maps
- `test/java7/integration_all_features.java` - Combine all features

### 5.2 Bytecode Verification

For each test, verify bytecode with `javap -v`:
- Check class file version: 51.0
- Verify StackMapTable attributes present
- Verify correct desugaring (string switch, try-with-resources)
- Verify constant pool entries

### 5.3 JVM Compatibility Testing

Test with multiple JVMs:
- OpenJDK 7, 8, 11, 17, 21
- Verify all bytecode loads and executes correctly
- Verify no verification errors

### 5.4 Regression Testing

Ensure all existing Java 5 and Java 6 tests still pass:
- Run full test suite after each feature implementation
- Fix any regressions immediately

---

## Implementation Dependencies

### Dependency Graph

```
Phase 1: Language Features (Can be mostly parallel)
├── 1.1 Binary Literals & Underscores (independent)
├── 1.2 Strings in Switch (independent)
├── 1.3 Diamond Operator (requires generics - already done)
├── 1.4 Multi-Catch (independent)
├── 1.5 Precise Rethrow (soft dependency on multi-catch)
├── 1.6 Try-with-Resources (requires AutoCloseable runtime)
└── 1.7 SafeVarargs (requires annotation support - already done)

Phase 2: Bytecode Version 51
├── 2.1 Version Update (independent, but should be first)
└── 2.2 StackMapTable (requires all bytecode generation working)

Phase 3: invokedynamic (DEFERRED)
├── 3.1 invokedynamic instruction (requires constant pool changes)
└── 3.2 Method Handles (DEFERRED - library work)

Phase 4: Runtime Library (Can be parallel)
├── 4.1 AutoCloseable (required for try-with-resources)
├── 4.2 Throwable.addSuppressed() (required for try-with-resources)
├── 4.3 SafeVarargs (required for SafeVarargs annotation)
└── 4.4 Objects utility class (independent)

Phase 5: Testing (After each feature)
└── Continuous integration testing
```

### Critical Path

1. **Phase 2.1:** Bytecode version 51.0 update (enables target 1.7)
2. **Phase 4.1, 4.2:** AutoCloseable and Throwable updates (blocks try-with-resources)
3. **Phase 1 Features:** Implement in priority order
4. **Phase 2.2:** StackMapTable generation (required for Java 7 compliance)
5. **Phase 5:** Comprehensive testing

---

## Risk Assessment

### High-Risk Components

1. **StackMapTable Generation (Phase 2.2)**
   - **Risk:** Most complex feature, potential for subtle bugs
   - **Mitigation:**
     - Implement incrementally (simple methods first)
     - Extensive testing with `javap -v` verification
     - Test with multiple JVMs
     - Study reference implementations (OpenJDK, Eclipse JDT)

2. **Try-with-Resources Desugaring (Phase 1.6)**
   - **Risk:** Complex control flow, multiple exception paths
   - **Mitigation:**
     - Follow JLS specification exactly
     - Test all exception scenarios
     - Verify suppressed exception handling
     - Compare bytecode with javac output

3. **String Switch Desugaring (Phase 1.2)**
   - **Risk:** Hash collision handling, correct equals() behavior
   - **Mitigation:**
     - Test with strings that have hash collisions
     - Test null handling
     - Verify optimization (tableswitch vs lookupswitch)

### Medium-Risk Components

1. **Diamond Operator (Phase 1.3)**
   - **Risk:** Type inference complexity with nested generics
   - **Mitigation:** Leverage existing generics implementation, extensive testing

2. **Precise Rethrow (Phase 1.5)**
   - **Risk:** Flow analysis complexity
   - **Mitigation:** Conservative analysis, fallback to declared types

### Low-Risk Components

1. Binary literals and underscores (Phase 1.1)
2. Multi-catch (Phase 1.4)
3. SafeVarargs (Phase 1.7)
4. Runtime library updates (Phase 4)

---

## Implementation Timeline Estimate

### Optimistic Timeline (Focused Development)

- **Phase 1.1:** 2-3 days (Binary literals, underscores)
- **Phase 1.2:** 5-7 days (Strings in switch)
- **Phase 1.3:** 7-10 days (Diamond operator)
- **Phase 1.4:** 5-7 days (Multi-catch)
- **Phase 1.5:** 4-5 days (Precise rethrow)
- **Phase 1.6:** 10-14 days (Try-with-resources)
- **Phase 1.7:** 2-3 days (SafeVarargs)
- **Phase 2.1:** 1-2 days (Version update)
- **Phase 2.2:** 20-30 days (StackMapTable)
- **Phase 4:** 3-5 days (Runtime library)
- **Phase 5:** 10-15 days (Testing, debugging, refinement)

**Total: 69-101 days (approximately 3-5 months of focused work)**

### Realistic Timeline (With Debugging, Iteration)

**Total: 4-6 months**

---

## Recommended Implementation Order

### Stage 1: Foundation (Weeks 1-2)
1. Bytecode version 51.0 update (Phase 2.1)
2. Runtime library updates (Phase 4)
3. Testing infrastructure setup

### Stage 2: Low-Hanging Fruit (Weeks 3-4)
1. Binary literals and underscores (Phase 1.1)
2. SafeVarargs annotation (Phase 1.7)
3. Multi-catch (Phase 1.4)

### Stage 3: Medium Complexity (Weeks 5-8)
1. Strings in switch (Phase 1.2)
2. Diamond operator (Phase 1.3)
3. Precise rethrow (Phase 1.5)

### Stage 4: High Complexity (Weeks 9-14)
1. Try-with-resources (Phase 1.6)
2. StackMapTable generation (Phase 2.2)

### Stage 5: Testing and Refinement (Weeks 15-16)
1. Comprehensive testing (Phase 5)
2. Bug fixes and optimization
3. Documentation updates

---

## References

### Java Specifications
- **JLS 7:** Java Language Specification, Java SE 7 Edition
  - https://docs.oracle.com/javase/specs/jls/se7/html/index.html
- **JVMS 7:** Java Virtual Machine Specification, Java SE 7 Edition
  - https://docs.oracle.com/javase/specs/jvms/se7/html/index.html

### JSR Specifications
- **JSR 336:** Java SE 7 Release Contents
- **JSR 334:** Small Enhancements to the Java Programming Language (Project Coin)
- **JSR 292:** Supporting Dynamically Typed Languages on the Java Platform
- **JSR 203:** More New I/O APIs for the Java Platform (NIO.2)

### Implementation References
- **OpenJDK 7 Source Code:** https://github.com/openjdk/jdk7u
- **Eclipse JDT Compiler:** https://github.com/eclipse-jdt/eclipse.jdt.core
- **Project Coin Documentation:** https://openjdk.org/projects/coin/

---

## Success Criteria

**Java 7 implementation is complete when:**

1. ✅ All JSR 334 (Project Coin) features implemented and tested
2. ✅ Bytecode version 51.0 generated correctly
3. ✅ StackMapTable attributes generated for all methods
4. ✅ All generated class files load and execute on Java 7+ JVMs
5. ✅ Comprehensive test suite passes (50+ Java 7 tests)
6. ✅ No regressions in Java 5/6 tests
7. ✅ Documentation updated (README, implementation status)

**Optional (Deferred to Java 8):**
- invokedynamic instruction support
- java.lang.invoke package
- NIO.2 library additions

---

## Conclusion

Java 7 implementation is a substantial but achievable project. The roadmap prioritizes:

1. **Foundation first:** Bytecode version, runtime library
2. **Quick wins:** Simple features for early progress
3. **Core features:** Project Coin language enhancements
4. **Complex features last:** StackMapTable, try-with-resources
5. **Continuous testing:** Validate with real JVM at each step

**Deferred to Later:**
- invokedynamic and Method Handles (not used by Java 7 source)
- NIO.2 library (runtime library, not compiler work)

With focused effort, JOPA can achieve comprehensive Java 7 support, continuing its mission as a modern, bootstrappable Java compiler.

---

**Document Version:** 1.0
**Last Updated:** 2025-01-23
**Author:** Claude (Anthropic)
**Status:** Draft - Ready for Review
