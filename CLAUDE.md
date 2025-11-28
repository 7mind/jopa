# JOPA Project Guidelines

## Build System
- Clang-based C++ project
- Use CMake for building

## Debugging
- Use LLDB (not GDB) - this is a clang project
- Example: `lldb build/src/jopa -- <args>`
- Use Valgrind
- Use execinfo.h and arg2line
- You may replace JAVAC command in  vendor/CMakeLists.txt: like "JAVAC=valgrind --error-exitcode=1 --log-file=${CMAKE_BINARY_DIR}/valgrind-%p.log ${JOPA_EXECUTABLE} --nowarn:unchecked -bootclasspath ${JOPA_RUNTIME_JAR}"
- REMEMBER: GNU Classpath builds fine with Javac. If we encounter a semantic error there, our compiler is flawed and we need to fix it, not mask issues!
- Never ignore pre-existing bugs you discover, we need to fix ALL the bugs (and keep a log of discovered/fixed ones)
- Don't ignore pre-existing bugs, fix them!
- Use jopa-stub-rt for GNU Classpath bootstrap! Don't forget to add missing stubs into jopa-stub-rt!
- When working on GNU Classpath bootstrap, concentrate on fixing semantic errors first, not crashes!
- When you see a class of semantic errors, reproduce with an isolated self-contained test with main method, integrate that test into primary test suite and then fix!
- In order to integrate a new test, put it into ./test directory and register in test/CMakeLists.txt with `add_jopa_run_test` directive
- Any issues in primary test suite always MUST be fixed before we proceed with GNU classpath, regardless if they are pre-existing
- For any semantic errors we should always start by checking the correctness of our stub runtime and extend it if necessary! Don't forget to always rebuild the stub runtime jar after modifications before running any tests. 
- Only use stub runtime jar for tests, don't use system rt.jar or directories on the disk

## ASAN/UBSAN Build (recommended for debugging)
- Always use sanitizer-enabled build when debugging memory issues
- Configure with debug info for readable stack traces:
  ```
  cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
    -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
    -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
  ```
- For bootstrap builds, set env vars to allow compilation to continue past non-fatal errors:
  - `ASAN_OPTIONS=detect_leaks=0` (leak detection fails configure checks)
  - `UBSAN_OPTIONS=halt_on_error=0` (some UB is tolerable during bootstrap)

## Compiler Testing
- Run tests with: `ctest --test-dir build --output-on-failure`
- Bootstrap build target: `cmake --build build --target vendor_jvm`
