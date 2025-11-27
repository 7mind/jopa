# JOPA Project Guidelines

## Build System
- Clang-based C++ project
- Use CMake for building

## Debugging
- Use LLDB (not GDB) - this is a clang project
- Example: `lldb build/src/jopa -- <args>`
- Use Valgrind
- Use execinfo.h and arg2line

## Compiler Testing
- Run tests with: `ctest --test-dir build --output-on-failure`
- Bootstrap build target: `cmake --build build --target vendor_jvm`
