# JDK7 Compliance Failures (post-DA fixes)

Source logs:
- Detailed per-test first 40 lines: `/tmp/jopa-jdk7-failure-details.log`
- Failing list: `build/compliance_jdk7/failed.txt`
- Run: `scripts/test-java7-compliance.sh` (680/846 pass; 166 fail)

## Category Summary

- APT mirror API stubs returning raw Object (6)
- Annotation processing API gaps (≈27)
- Generics/type-system deficiencies (≈42)
- Varargs crashers (2)
- Multi-catch / try-with-resources missing (4)
- Javadoc harness/doclet stubs broken (20)
- Tooling API stubs missing (javac/javap/javah) (≈14)
- Runtime stub gaps (3+)
- Missing test deps/classpath (4)
- Additional crash: `EmitCast` assertion (1)

## Details by Category

### APT mirror API stubs returning raw Object
- `tools/apt/mirror/declaration/EnumDecl.java`: `getParameters()` missing on return type (Object).
- `ConstructorDecl.java`, `ParameterDecl.java`, `MethodDecl.java`: iterator().next() yields Object, not specific declaration types.
- `PackageDecl.java`: `PackageDeclaration.getQualifiedName()` missing.
- `mirror/util/Overrides.java`: `getSimpleName()` missing on parameters.

### Annotation processing API gaps
- Missing/incorrect APIs: `Filer.createResource`, `Elements.typesIn/methodsIn/constructorsIn`, `TypeKindVisitor7`, `ForwardingJavaFileManager`, `JavacTask` typed access (`getElements()`), `Enum.valueOf(Class,String)`, `System.setSecurityManager`, others.
- Frequent type-mismatch: `JavaCompiler.CompilationTask` not assignable to `JavacTask`.
- Crash: `processing/model/util/elements/TestGetConstantExpression.java` → `EmitCast` assertion (bytecode_expr.cpp:1960).

### Generics / type-system deficiencies
- Inference collapses to `Object`, violating bounds/return types (e.g., `generics/inference/T6650759*`, `T6938454a`, `T6995200`).
- Missing generic signatures/overloads: `Arrays.fill(Object[],int,int,...)`, `Collections.reverseOrder()`, `Class.cast`, `EnumMap`, `Hashtable`, `Queue`, `LinkedHashSet`, etc.
- Access checks: protected `clone`, unimplemented abstract methods (e.g., `T4711694`).
- Crashes: `types/CastTest.java`, `PrimitiveConversionTest.java`, `GenericTypeWellFormednessTest.java`, `BoxingConversionTest.java` → `MethodSymbol::Header` assertion.

### Varargs crashers
- `varargs/5088429/T5088429Pos01.java`, `T5088429Pos02.java`: `Tuple<VariableSymbol>` bounds assertion (tuple.h:158).

### Multi-catch / try-with-resources missing
- `multicatch/Pos05.java`, `Pos07.java`, `7005371/T7005371.java`, `TryWithResources/TwrMultiCatch.java`: grammar/semantics unimplemented → many semantic errors.

### Javadoc harness/doclet stubs broken
- Common: `Tester$Doclet` not found, `Tester` abstract/missing `verify/printClass`.
- Bad `Main.execute(...)` overload resolution (`parser/7091528/T7091528.java`, `nestedClass/NestedClass.java`).
- Affects ~20 tests under `tools/javadoc/**` (generics, annotations, varArgs, enum doc tests).

### Tooling API stubs missing (javac/javap/javah)
- `javac/6567415/T6567415.java`: `ClassReader.INITIAL_BUFFER_SIZE` missing.
- `javap/*.java`, `javah/*.java`: `com.sun.tools.javap.Main`, `javah.Main` unresolved; `com.sun.tools.classfile.*` package missing (`javap/classfile/deps/GetDeps.java`).
- Javadoc tool helpers (`BooleanConst`, `MethodLinks`, `XWerror`, `NoStar`) and doclet executes complaining about missing `Main` overloads.

### Runtime stub gaps
- `Paths/CompileClose.java`: `URLClassLoader`, `File.toURL`, `Class.forName(String,boolean,ClassLoader)` overload missing.
- `javah/T7126832/T7126832.java`: `java.io`, `java.util` stubs (File, Locale, etc.) missing in runtime path.
- `javah/6572945/T6572945.java`: `ProcessBuilder`/`Process` missing.
- `javadoc/6176978/T6176978.java`: similar runtime API holes.

### Missing test deps / classpath setup
- `6835430/T6835430.java`: helper package `B` missing on classpath.
- `DuplicateImport.java`: expects synthetic package `nonexistent.pack`.
- `T5090006/T5090006.java`: needs `stub_tie_gen` and `testutil` sources.
- `nio/compileTest/CompileTest.java`: `Paths/` helpers (`SameJVM`) missing on sourcepath/classpath.

### Additional crash
- `processing/model/util/elements/TestGetConstantExpression.java`: `EmitCast` assertion (bytecode_expr.cpp:1960) — listed above but called out separately as a stopper.

## Artifact Pointers
- Full log: `/tmp/jopa-jdk7-failure-details.log`
- Fail list: `build/compliance_jdk7/failed.txt`
- Per-category counts (approx):
  - APT mirror: 6
  - Annotation processing: ~27
  - Generics/types: ~42
  - Varargs crashes: 2
  - Multi-catch/TWR: 4
  - Javadoc harness: 20
  - Tooling stubs (javac/javap/javah): ~14
  - Runtime stub gaps: 3+
  - Missing deps/classpath: 4
  - Internal assertions: 7 (overlaps categories above)
