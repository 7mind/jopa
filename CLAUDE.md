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

## Compiler Testing
- Run tests with: `ctest --test-dir build --output-on-failure`
- Bootstrap build target: `cmake --build build --target vendor_jvm`
