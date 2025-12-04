# JOPA Project Guidelines

JOPA is a Java compiler written in C++.

## Core Principles

- Avoid workarounds and temporary solutions. If you must, mark the line with a TODO and track it in `./TODO.md`
- Never ignore pre-existing bugs - fix them and keep a log of discovered/fixed ones
- GNU Classpath builds fine with Javac. If we encounter a semantic error there, our compiler is flawed and we need to fix it, not mask issues
- Always pipe build/test logs into a log in /tmp through tee

## Build System

- Clang-based C++ project using CMake
- Standard build: `cmake -S . -B build && cmake --build build`
- Compiler build: `cmake --build build --target jopa`
- Run tests: `ctest --test-dir build --output-on-failure`
- Bootstrap build: `cmake --build build --target jamvm_with_gnucp`

## Debugging

### Tools
- Use LLDB (not GDB) - this is a clang project
- Use Valgrind for memory debugging
- Use execinfo.h and addr2line for stack traces

### LLDB Example
```bash
lldb build/src/jopa -- <args>
```

### Valgrind with Bootstrap
Replace JAVAC command in `vendor/CMakeLists.txt`:
```
JAVAC=valgrind --error-exitcode=1 --log-file=${CMAKE_BINARY_DIR}/valgrind-%p.log ${JOPA_EXECUTABLE} --nowarn:unchecked -bootclasspath ${JOPA_RUNTIME_JAR}
```

### Debug Build with Sanitizers
Debug builds automatically enable ASAN, UBSAN, and leak detection.
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

To opt-out of leak detection:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DJOPA_ENABLE_LEAK_SANITIZER=OFF
```

For bootstrap builds, UBSAN errors are non-fatal by default in vendor/CMakeLists.txt.

## Testing

### Adding New Tests
1. Create test file in `./test` directory
2. Register in `test/CMakeLists.txt` with `add_jopa_run_test` directive
3. Only use stub runtime jar for tests - never system rt.jar or directories on disk

### Test Workflow
- Issues in primary test suite MUST be fixed before proceeding with GNU Classpath
- When you see a class of semantic errors:
  1. Reproduce with an isolated self-contained test with main method
  2. Integrate that test into primary test suite
  3. Fix the issue

For comprehensive JDK compliance testing, use the `scripts/compliance_tester.py` tool with the `--classpath` option to specify the runtime environment (e.g., `--classpath gnucp` or `--classpath stub`).

## GNU Classpath Bootstrap

### Priority Order
1. Fix semantic errors first, not crashes
2. Always check stub runtime correctness first for semantic errors
3. Extend jopa-stub-rt as necessary

### Stub Runtime Workflow
1. Add missing stubs to jopa-stub-rt
2. Rebuild the stub runtime jar
3. Run tests (always rebuild before testing!)

### Features
- Scans JDK test directories (e.g., `assets/jdk7u-langtools/test/tools/javac`) for valid tests.
- Filters tests based on instructions (supports `@run main`, `@compile`, and `@library` directives).
- Supports blacklisting flaky or irrelevant tests via multiple files.
- Configurable classpath (stub runtime, GNU Classpath, or custom).
- Test mode selection: run tests or compile-only.
- Executes tests in parallel using a thread pool.
- Provides detailed statistics and failure breakdowns.

### Usage

The tool operates in two main modes: `--prepare` (to generate a test list) and `--test` (to execute tests).

#### 1. Prepare Mode
Scans the asset directories and generates a whitelist of tests to run.

```bash
./scripts/compliance_tester.py --prepare --jdk <7|8> [--blacklist <file>]... [--testlist <output_file>]
```

- `--jdk {7,8}`: **Required**. Specifies which JDK version assets to scan.
- `--blacklist <file>`: Optional. Path to a file containing substrings to blacklist (default: `./scripts/test-blacklist.txt` if no other blacklist provided). Can be specified multiple times to use multiple files.
- `--testlist <output_file>`: Optional. Path to write the generated whitelist (default: `scripts/jdk{7|8}-test-whitelist.txt` based on `--jdk`).

**Example:**
```bash
# Prepare tests for JDK 7, writing to scripts/jdk7-test-whitelist.txt (default)
./scripts/compliance_tester.py --prepare --jdk 7
# Prepare tests with extra blacklist
./scripts/compliance_tester.py --prepare --jdk 8 --blacklist scripts/test-blacklist.txt --blacklist my-flaky-tests.txt
```

#### 2. Test Mode
Executes the tests using the specified compiler and JVM.

```bash
./scripts/compliance_tester.py --test [--jdk <7|8> | --testlist <input_file>] [options]
```

- `--testlist <input_file>`: Path to the file containing the list of tests to run (default: `scripts/jdk{7|8}-test-whitelist.txt` based on `--jdk`).
- `--jdk {7,8}`: Used to determine the default testlist path if `--testlist` is not provided. Also implicitly used to determine scan roots if no `--testlist` is provided.
- `--limit <N>`: Limit the run to N random tests (useful for quick checks).
- `--mode <run|compile>`: Test mode. `run` (default) compiles and executes the test. `compile` only checks for successful compilation.
- `--timeout <seconds>`: Set timeout for compilation and execution (default: 5s).
- `--classpath <target>`: Specify runtime classpath. Options:
    - `gnucp`: Use JOPA-built GNU Classpath (default).
    - `stub`: Use the JOPA stub runtime jar.
    - `<path>`: Custom path to a classpath entry (jar or directory).
- `--compiler <path>`: Path to the javac-like compiler (default: `javac`).
- `--jvm <path>`: Path to the java runtime (default: `java`).
- `--arg <value>`: Pass arguments to the compiler (can be used multiple times).
- `--no-success`: Suppress logs for successful tests.
- `--verbose-failures`: Print stdout/stderr for failed tests.

**Example:**
```bash
# Run 50 random tests for JDK 7 using JOPA with stub runtime
./scripts/compliance_tester.py --test --jdk 7 --limit 50 --compiler ./build/src/jopa --timeout 10 --no-success --verbose-failures --classpath stub
```

### Output Format
The tool provides feedback on the execution progress and results:
- **Progress Log:** `[Index/Total] TestPath (TempPath): OUTCOME (Reason)`
- **Outcomes:** `SUCCESS`, `FAILURE`, `TIMEOUT`, `CRASH`.
- **Summary:** Total counts, success/failure rates, and a breakdown of failure reasons (e.g., "compiler failed", "test timed out").
