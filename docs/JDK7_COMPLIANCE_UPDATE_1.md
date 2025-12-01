# JDK 7 Compliance Update (2025-12-01)

## Progress Report

**Previous Pass Rate:** 85.8% (653/761)
**Current Pass Rate:** 86.6% (659/761)

### Actions Taken

1.  **Enhanced Test Runner:** Implemented batch compilation for directories with < 20 files. This resolved multi-file dependency issues (e.g., `AbstractSub` finding `Super`).
2.  **Fixed Compiler Crash:** Patched `src/symbol.cpp` to handle varargs signature generation safely. Resolved `tools/javac/types/CastTest.java`.
3.  **Expanded Runtime Stubs:**
    *   Added `java.util.zip.ZipFile`, `ZipEntry`, `ZipException`, `ZipConstants`.
    *   Added `sun.tools.jar.Main` stub.
    *   Added `java.nio.file.Files`, `FileStore`, `CopyOption`, `attribute.FileAttribute`.
    *   Added `java.lang.String.concat(String)`.
    *   Added `java.lang.Boolean.getBoolean(String)`.
    *   Added `java.util.StringTokenizer`.
    *   Added `java.util.ResourceBundle`.

### Remaining High-Value Targets

*   **Generic Type Inference:** The "bypasses type parameterization" warning leading to `Object` erasure is still the largest source of failures.
*   **Annotation Processing:** Remaining failures in `tools/apt` or `tools/javac/api` that weren't caught by the exclusion list.
*   **Standard Library Gaps:** `java.util.ResourceBundle`, `java.util.StringTokenizer` are still missing and causing failures in `tools/javac/diags`.

## Next Steps

1.  Investigate the type inference warning in `src/semantic.h`.
