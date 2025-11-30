# Jopa Compiler Improvements

## JDK 7 Compliance Progress

We have improved JDK 7 compliance from 66.9% to 75.0%.

### Major Changes

1.  **Runtime Stubs Enhancement**:
    -   Added missing exception classes: `NoSuchMethodException`, `NoSuchFieldException`, `NegativeArraySizeException`.
    -   Updated `java.lang.Class` to include `throws ClassNotFoundException` in `forName`.
    -   Updated `java.lang.String` to include missing `indexOf` and `lastIndexOf` overloads.
    -   Updated `java.io.File` to include `deleteOnExit`.
    -   Added `java.awt` and `java.awt.event` stubs (`Window`, `Frame`, `Component`, `Container`, `WindowListener`, `WindowAdapter`).
    -   Added `javax.tools` and `com.sun.source` stubs to support compiler API tests (`JavacTask`, `Trees`, `TreePath`, `StandardLocation`).
    -   Added `com.sun.tools.javac.Main` stub.

2.  **Definite Assignment**:
    -   Fixed failures in `DUAssert.java` by adding primitive constructors to `java.lang.AssertionError`.

3.  **Test Infrastructure**:
    -   Updated `test-compliance.sh` to include `tools/javac/api/lib` in sourcepath, enabling many API tests to pass.

### Remaining Challenges

-   **Multi-file Tests**: Many tests (e.g., `ConstValInlining`, `T`, `EnclosingAccessCheck`) fail because they depend on auxiliary classes in the same directory or in `p` subdirectories, which are not automatically found or compiled by the current test runner.
-   **Internal API Dependencies**: Tests relying on deep internal APIs of `javac` (e.g., `com.sun.tools.javac.main.JavaCompiler`) require extensive stubbing.
-   **Bytecode Dependencies**: Tests depending on `.jasm` (Java Assembler) files cannot be compiled as we lack an assembler in the test loop.

### Next Steps

-   Expand `com.sun.javadoc` stubs to fix Javadoc tests.
-   Implement a more robust test runner that can handle multi-file compilations or dependencies in the same directory.
-   Continue refining `Definite Assignment` logic in `src/definite.cpp`.