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
Debug builds automatically enable ASAN and UBSAN. Leak detection is disabled by default.
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

To opt-in to leak detection:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DJOPA_ENABLE_LEAK_SANITIZER=ON
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

## GNU Classpath Bootstrap

### Priority Order
1. Fix semantic errors first, not crashes
2. Always check stub runtime correctness first for semantic errors
3. Extend jopa-stub-rt as necessary

### Stub Runtime Workflow
1. Add missing stubs to jopa-stub-rt
2. Rebuild the stub runtime jar
3. Run tests (always rebuild before testing!)
