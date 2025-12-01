# Jopa Compiler Improvements

## JDK 7 Compliance Progress

We have achieved 73.7% compliance (626 passing tests). While the absolute number dropped slightly from a peak of 75.0% due to the removal of unstable `com.sun.mirror` stubs that caused compiler crashes, the current state is more robust.

### Major Changes

1.  **Runtime Stubs Enhancement**:
    -   Added `com.sun.javadoc` package stubs (`RootDoc`, `ClassDoc`, `Tag`, etc.) enabling Javadoc tests (`AuthorDD`, `MetaTag`) to pass.
    -   Added `com.sun.tools.javadoc.Main` stub.
    -   Added `junit.framework` stubs (`TestCase`, `Assert`, etc.).
    -   Enhanced `java.io` stubs (`ObjectInputStream`, `ObjectOutputStream` implementing all abstract methods).
    -   Enhanced `java.net.URI` (`getPath`).
    -   Enhanced `javax.tools` (`DiagnosticCollector`, `StandardLocation`).
    -   Enhanced `com.sun.source.util` (`Trees`, `TreePath`).

2.  **Stability**:
    -   Removed `com.sun.mirror` (APT) stubs because they triggered a compiler crash (`runtime error: member call on null pointer`) in `EnumDecl.java`. This caused a drop in passing tests but resolved the crash.

### Remaining Challenges

-   **Compiler Crash**: The `jopa` compiler crashes during bytecode generation when compiling APT tests (`EnumDecl`) if `com.sun.mirror` stubs are present. This requires C++ debugging of `src/codegen/bytecode_expr.cpp`.
-   **Multi-file Tests**: Tests depending on auxiliary files in the same directory (e.g., `ConstValInlining`, `EnclosingAccessCheck`) fail because the test runner compiles files individually.
-   **Internal API Dependencies**: Tests relying on deep internal APIs of `javac` (`com.sun.tools.javac.main.JavaCompiler`) remain unsupported.

### Next Steps

-   Investigate and fix the compiler crash in `ByteCode::EmitName` to restore `com.sun.mirror` support.
-   Implement a test runner capable of multi-file compilation.
-   Continue expanding runtime stubs for `java.util` and other core packages.
