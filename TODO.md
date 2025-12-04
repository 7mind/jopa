# JOPA TODO

## Known Issues

### Class file corruption with Comparable<T> + CharSequence in java.lang

When a class in the `java.lang` package implements both `Comparable<T>` and `CharSequence`,
and is compiled with `-sourcepath runtime`, the generated class file is corrupted.

**Reproduction:**
```bash
# This produces a corrupted class file:
./build/src/jopa -source 1.5 -sourcepath runtime -d /tmp/out runtime/java/lang/String.java
# With String.java implementing: java.io.Serializable, Comparable<String>, CharSequence
```

**Workaround:**
`java.lang.String` in stub runtime only implements `Serializable` and `CharSequence`, not `Comparable<String>`.

**Impact:**
- JDK tests that use `Comparable<String>` methods on String will fail
- Code that requires `String implements Comparable<String>` won't compile with stub runtime

**To investigate:**
- Look at bridge method generation when compiling java.lang classes
- The issue may be related to how the compiler handles self-referential generics in bootstrap mode

### Target 1.7 StackMapTable generation OOM

Compiling tests with `-target 1.7` causes out-of-memory errors on tests with many boolean method arguments (e.g., `NumericBitwiseBoxingTest.java` with ~47 test calls).

**Reproduction:**
```bash
./build/src/jopa -source 1.7 -target 1.7 -classpath ./build/runtime/jopa-stub-rt.jar \
    -d /tmp/out test/autoboxing/NumericBitwiseBoxingTest.java
# Process consumes 18GB+ RAM before being killed
```

**Impact:**
- Target 1.7 excluded from test matrix
- StackMapTable generation for class version 51.0 has memory leak

**To investigate:**
- Memory leak is NOT in: SaveLocals, RecordFrame, PushType, SetLocal (debug confirmed reasonable counts)
- Issue manifests when combining many int boxing tests with long boxing tests
- Suspected: exponential growth in some path during type verification inference

## JDK 7 Compliance (In Progress)

- **Status**: 77.4% (657/849 tests passed).
- **Achievements**:
    - Fixed critical compiler crash in `ByteCode::EmitName` (null symbol check).
    - Fixed crash in `Semantic::CanCastConvert` (null symbol check).
    - Fixed UBSan error in `double.cpp` by avoiding large constants in stubs.
    - Expanded runtime stubs: `com.sun.mirror` (APT), `com.sun.javadoc`, `javax.tools`, `java.io`, `java.util`.
- **Remaining Issues**:
    - `tools/apt` tests fail due to type inference issues on stubs (`iterator().next()` returning Object).
    - Multi-file tests fail due to test runner limitations.
    - `tools/javac` internal API dependencies.
