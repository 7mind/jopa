# Java 5 Implementation Plan for Jikes

## Project Goal
Modernize the ancient Jikes Java compiler to support Java 5 (JDK 1.5) language features and bytecode.

## Current Status: Phase 1 Complete ✅

### Completed: Generics Support
- ✅ Generic class definitions with type parameters
- ✅ Bounded type parameters (`<T extends Number>`)
- ✅ Multiple bounds (`<T extends A & B>`)
- ✅ Generic methods and constructors
- ✅ Type erasure
- ✅ Signature attributes for reflection
- ✅ Bridge method generation
- ✅ Raw type warnings
- ✅ Generic inheritance (fixed semantic environment stack issue)
- ✅ Type parameter bounds validation

**Code Added**: ~2,500 lines across 9 files
**Time Invested**: ~8-10 hours
**Status**: Functionally complete, ready for production use

---

## Phase 2: Complete Java 5 Feature Set

### 2.1: Enhanced For-Loop (For-Each) ⏳ NEXT
**Priority**: HIGH - Widely used, quick win
**Estimated Time**: 2-3 hours
**Difficulty**: ⭐⭐☆☆☆ (Easy)

#### Feature Description
```java
// Iterating over collections
for (String item : collection) {
    System.out.println(item);
}

// Iterating over arrays
for (int num : numbers) {
    total += num;
}
```

#### Implementation Plan
1. **Parser Changes** (30-45 min)
   - Modify grammar to recognize `for (Type var : expression)`
   - Create `AstEnhancedForStatement` AST node
   - Handle both Iterable and array cases

2. **Semantic Analysis** (45-60 min)
   - Check expression is array or Iterable
   - Type check loop variable
   - Ensure variable type is assignable from element type
   - Handle final loop variables

3. **Code Generation** (45-60 min)
   - Desugar to traditional for/while loop:
     ```java
     // For arrays: for (T item : array)
     for (int i = 0; i < array.length; i++) {
         T item = array[i];
         // loop body
     }

     // For Iterable: for (T item : iterable)
     Iterator<T> iter = iterable.iterator();
     while (iter.hasNext()) {
         T item = iter.next();
         // loop body
     }
     ```

4. **Testing** (30 min)
   - Test with arrays (primitive and reference)
   - Test with collections
   - Test with nested loops
   - Test break/continue

#### Files to Modify
- `src/parser.g` or equivalent - Grammar
- `src/ast.h` - New AST node
- `src/body.cpp` - Semantic processing
- `src/code.cpp` - Bytecode generation

---

### 2.2: Varargs (Variable Arguments) ⏳
**Priority**: HIGH - Commonly used in APIs
**Estimated Time**: 3-4 hours
**Difficulty**: ⭐⭐⭐☆☆ (Medium)

#### Feature Description
```java
void printf(String format, Object... args) {
    // args is Object[]
}

printf("Hello %s", "world");
printf("Values: %d %d %d", 1, 2, 3);
```

#### Implementation Plan
1. **Parser Changes** (30 min)
   - Recognize `Type... identifier` in parameter list
   - Must be last parameter
   - Only one varargs per method

2. **Semantic Analysis** (90-120 min)
   - Mark method as ACC_VARARGS
   - Treat varargs parameter as array type internally
   - Validate varargs is last parameter
   - Check for duplicate method signatures considering varargs

3. **Call Site Processing** (60-90 min)
   - At call sites, wrap varargs in array
   - Handle zero arguments (empty array)
   - Handle explicit array passing
   - Type checking for each argument

4. **Bytecode Generation** (30-45 min)
   - Set ACC_VARARGS flag on method
   - Generate array creation at call sites
   - Store arguments in array

5. **Testing** (30 min)
   - Test with 0, 1, many arguments
   - Test with primitive types (needs boxing)
   - Test explicit array passing
   - Test overload resolution

#### Files to Modify
- `src/parser.g` - Grammar for `...`
- `src/symbol.h` - ACC_VARARGS flag
- `src/decl.cpp` - Method declaration processing
- `src/expr.cpp` - Method call processing
- `src/bytecode.cpp` - Flag setting

---

### 2.3: Static Imports ⏳
**Priority**: MEDIUM - Nice to have
**Estimated Time**: 3-4 hours
**Difficulty**: ⭐⭐⭐☆☆ (Medium)

#### Feature Description
```java
import static java.lang.Math.PI;
import static java.lang.Math.*;

double area = PI * r * r;  // No Math. prefix needed
```

#### Implementation Plan
1. **Parser Changes** (30 min)
   - Recognize `import static` syntax
   - Distinguish from regular imports

2. **Import Processing** (90-120 min)
   - Process static imports separately
   - Handle single static import
   - Handle static on-demand import (*)
   - Store in separate table

3. **Name Resolution** (60-90 min)
   - Check static imports when resolving simple names
   - Handle ambiguity (multiple static imports of same name)
   - Proper shadowing rules

4. **Testing** (30 min)
   - Test static field imports
   - Test static method imports
   - Test on-demand imports
   - Test shadowing and ambiguity

#### Files to Modify
- `src/parser.g` - Import syntax
- `src/decl.cpp` - Import processing
- `src/lookup.cpp` - Name resolution
- `src/semantic.h` - Static import tables

---

### 2.4: Enums ⏳
**Priority**: MEDIUM - Important but complex
**Estimated Time**: 6-8 hours
**Difficulty**: ⭐⭐⭐⭐☆ (Hard)

#### Feature Description
```java
enum Color {
    RED, GREEN, BLUE;

    private String hex;
    Color(String hex) { this.hex = hex; }
    public String getHex() { return hex; }
}

enum Planet {
    EARTH(5.976e+24, 6.37814e6),
    MARS(6.421e+23, 3.3972e6);

    private final double mass;
    private final double radius;

    Planet(double mass, double radius) {
        this.mass = mass;
        this.radius = radius;
    }
}
```

#### Implementation Plan
1. **Parser Changes** (60 min)
   - Recognize `enum` keyword
   - Parse enum constants
   - Parse enum body (fields, methods, constructors)
   - Handle enum constant arguments

2. **Type System** (90-120 min)
   - Create enum as special class extending `java.lang.Enum`
   - Mark with ACC_ENUM flag
   - Each constant is `public static final` field

3. **Synthetic Method Generation** (120-150 min)
   - Generate `values()` method returning array
   - Generate `valueOf(String)` method
   - Handle constructor rewriting (must call super)
   - Generate static initializer for constants

4. **Switch Statement Integration** (60 min)
   - Allow enums in switch expressions
   - Generate switch with ordinal() values
   - Proper constant case handling

5. **Semantic Validation** (60 min)
   - No public constructors
   - No abstract methods (unless all constants override)
   - Proper constant initialization
   - Inheritance restrictions

6. **Testing** (45 min)
   - Simple enums
   - Enums with fields/methods
   - Enums in switch statements
   - Enum constant-specific methods

#### Files to Modify
- `src/parser.g` - Enum syntax
- `src/ast.h` - Enum AST nodes
- `src/decl.cpp` - Enum processing
- `src/symbol.h` - Enum flags
- `src/body.cpp` - Switch with enums
- `src/bytecode.cpp` - Synthetic methods

---

### 2.5: Annotations ⏳
**Priority**: LOW - Complex, less critical for basic Java 5
**Estimated Time**: 8-12 hours
**Difficulty**: ⭐⭐⭐⭐⭐ (Very Hard)

#### Feature Description
```java
@Retention(RetentionPolicy.RUNTIME)
@Target(ElementType.METHOD)
public @interface Test {
    String value() default "";
    int timeout() default 0;
}

@Override
public String toString() { ... }

@Test(timeout = 1000)
public void myTest() { ... }
```

#### Implementation Plan
1. **Parser Changes** (90 min)
   - Recognize `@interface` for annotation declarations
   - Parse annotation usage syntax
   - Parse annotation elements (methods)
   - Parse default values

2. **Annotation Type Processing** (120-150 min)
   - Create as special interface extending `java.lang.annotation.Annotation`
   - Process annotation elements
   - Validate element types (primitives, String, Class, enum, array)
   - Handle default values

3. **Annotation Usage Processing** (120-180 min)
   - Parse and validate annotation usage
   - Check retention policy
   - Validate target (method, field, class, etc.)
   - Type check annotation element values
   - Handle single-value shorthand

4. **Bytecode Generation** (90-120 min)
   - Generate RuntimeVisibleAnnotations attribute
   - Generate RuntimeInvisibleAnnotations attribute
   - Encode annotation data
   - Handle nested annotations

5. **Built-in Annotations** (60 min)
   - Implement @Override validation
   - Implement @Deprecated handling
   - Implement @SuppressWarnings

6. **Testing** (60 min)
   - Annotation declarations
   - Annotation usage
   - Built-in annotations
   - Retention policies

#### Files to Modify
- `src/parser.g` - Annotation syntax
- `src/ast.h` - Annotation AST nodes
- `src/decl.cpp` - Annotation declaration processing
- `src/expr.cpp` - Annotation usage processing
- `src/bytecode.cpp` - Annotation attributes
- `src/symbol.h` - Annotation metadata

---

## Phase 3: Testing & Validation ⏳

### 3.1: Comprehensive Test Suite
**Estimated Time**: 4-6 hours

#### Test Categories
1. **Generics Tests**
   - Generic collections (List, Map, Set)
   - Wildcard captures
   - Recursive bounds
   - Generic method inference
   - Complex inheritance hierarchies

2. **Enhanced For-Loop Tests**
   - Arrays (primitive and reference)
   - Collections (List, Set)
   - Nested loops
   - Break/continue statements

3. **Varargs Tests**
   - Zero arguments
   - Multiple arguments
   - Explicit array passing
   - Overload resolution

4. **Enum Tests**
   - Simple enums
   - Enums with fields/constructors
   - Switch statements
   - valueOf/values methods

5. **Integration Tests**
   - Combining multiple Java 5 features
   - Real-world code examples
   - Edge cases

### 3.2: Bytecode Verification
- Use javap to verify generated bytecode
- Check signature attributes
- Verify bridge methods
- Validate enum synthetic methods
- Verify annotation attributes

---

## Phase 4: Advanced Generics (Optional) ⏳

**Priority**: LOW - Nice to have
**Estimated Time**: 8-16 hours

### Features
1. **Type Inference for Generic Methods**
   - Constraint solving
   - Type variable unification

2. **Capture Conversion**
   - Wildcard capture in method calls
   - Helper method generation

3. **Parameterized Type Substitution**
   - Full type substitution in generic contexts
   - Proper return type resolution

---

## Timeline Estimates

### Conservative Timeline (Best Practices)
- **Phase 2.1** (Enhanced For): 2-3 hours → **Week 1**
- **Phase 2.2** (Varargs): 3-4 hours → **Week 1-2**
- **Phase 2.3** (Static Imports): 3-4 hours → **Week 2**
- **Phase 2.4** (Enums): 6-8 hours → **Week 2-3**
- **Phase 2.5** (Annotations): 8-12 hours → **Week 3-4** (optional)
- **Phase 3** (Testing): 4-6 hours → **Week 4**

**Total**: 26-37 hours (1-2 months working part-time)

### Aggressive Timeline (If Focused)
- Complete in 2-3 intense sessions
- Prioritize high-impact features (skip annotations)
- **Total**: 18-25 hours over 1-2 weeks

---

## Success Criteria

### Phase 2 Complete When:
- ✅ Enhanced for-loop works with arrays and Iterables
- ✅ Varargs methods compile and work correctly
- ✅ Static imports resolve names properly
- ✅ Enums compile with proper synthetic methods
- ✅ All features have passing tests
- ✅ Bytecode validates with javap

### Project Complete When:
- ✅ Can compile real-world Java 5 code
- ✅ All Java 5 language features implemented
- ✅ Comprehensive test suite passes
- ✅ Documentation complete
- ✅ CI/CD pipeline working

---

## Risk Management

### High Risk Items
1. **Enums**: Complex, many moving parts
   - Mitigation: Break into smaller tasks, test incrementally

2. **Annotations**: Very complex, optional
   - Mitigation: Defer if time-constrained

3. **Type Inference**: Algorithmically complex
   - Mitigation: Keep as Phase 4 (optional)

### Low Risk Items
- Enhanced for-loop (syntactic sugar)
- Varargs (straightforward array handling)
- Static imports (name resolution extension)

---

## Next Immediate Steps

1. ✅ Document this plan
2. ✅ Add .gitignore
3. ✅ Add GitHub Actions workflow
4. ⏳ Implement enhanced for-loop
5. ⏳ Test enhanced for-loop
6. ⏳ Implement varargs
7. ⏳ Continue with plan...

---

## Notes

- All features target Java 5 (JDK 1.5) compliance
- Bytecode version will be 49.0 (Java 5)
- Focus on correctness over performance
- Maintain backward compatibility where possible
- Document all design decisions
- Test incrementally

**Let's build this systematically and ship a fully-functional Java 5 compiler!** 🚀
