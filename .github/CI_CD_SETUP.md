# CI/CD Setup for Jikes

## Overview

This repository uses GitHub Actions for continuous integration and testing. The workflow automatically builds Jikes using CMake and Nix, and runs the comprehensive Java 5 test suite on every push and pull request.

## Workflow: Build and Test

**File**: `.github/workflows/ci.yml`

### Triggers

- **Push** to branches: `main`, `master`, `develop`, `wip/cmake`
- **Pull Requests** to branches: `main`, `master`, `develop`
- **Manual**: `workflow_dispatch`

### Build Jobs

The workflow consists of two jobs:

1. **build-and-test**: Main CMake-based build with comprehensive test suite
2. **check-java5-features**: Feature matrix testing individual Java 5 features

### Workflow Steps (Main Job)

1. **Checkout Repository**
   - Uses `actions/checkout@v4`

2. **Install Nix**
   - Uses `DeterminateSystems/nix-installer-action@v14`
   - Enables reproducible builds

3. **Setup Nix Cache**
   - Uses `DeterminateSystems/magic-nix-cache-action@v8`
   - Speeds up builds

4. **Configure Build with CMake**
   - Runs CMake configuration with Java 5 support enabled

5. **Build Jikes**
   - Compiles using CMake
   - Creates `build/src/jikes` binary

6. **Run CTest**
   - Executes comprehensive test suite via CTest
   - Tests all Java 5 features:
     - Generics (type erasure, bounds, wildcards)
     - Enhanced for-loops
     - Varargs
     - Enums
     - Autoboxing/Unboxing
     - Static imports

7. **Generate Test Summary**
   - Creates GitHub step summary with test results
   - Always runs, even if tests fail

8. **Upload Artifacts**
   - Uploads compiled class files
   - Uploads test logs
   - Available for 7 days

## Local Testing

### Quick Test
```bash
nix develop
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJIKES_ENABLE_SOURCE_15=ON
cmake --build build -j$(nproc)
cd build
ctest --output-on-failure
```

### Manual Testing
```bash
# Build Jikes
nix develop
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJIKES_ENABLE_SOURCE_15=ON
cmake --build build -j$(nproc)

# Run individual test
./build/src/jikes -source 1.5 -sourcepath test-generics/runtime -d test-output test-generics/GenericTest.java

# Verify bytecode
ls -la test-output/*.class
javap -v test-output/GenericTest.class | grep Signature
```

## Test Suite

### Current Tests

1. **SimpleGeneric.java** - Basic generic class with one type parameter
2. **TwoTypeParams.java** - Generic class with multiple type parameters
3. **BoundedGeneric.java** - Generic class with bounded type parameter

### Adding New Tests

1. Create test file in `test-generics/`
2. Add test to `CMakeLists.txt` in the `jikes` directory
3. The test will automatically run in CI via CTest
4. Commit and push

Example:
```cmake
# Add to CMakeLists.txt
add_test(
  NAME compile_my_new_test
  COMMAND ${JIKES_BINARY} -source 1.5
          -sourcepath ${RUNTIME_PATH}:${TEST_GENERICS_PATH}
          -classpath ${RUNTIME_PATH}
          -d ${TEST_OUTPUT_DIR}
          ${TEST_GENERICS_PATH}/MyNewTest.java
)

add_test(
  NAME run_my_new_test
  COMMAND java -cp ${TEST_OUTPUT_DIR}:${RUNTIME_PATH} MyNewTest
)
set_tests_properties(run_my_new_test PROPERTIES DEPENDS compile_my_new_test)
```

## Workflow Status

You can view the workflow status:
- On GitHub: Repository → Actions tab
- Badge in README (to be added)

## Troubleshooting

### Build Failures

**Problem**: CMake configuration fails

**Solution**: Ensure CMake 3.15+ is available. Check that all dependencies are present in the Nix flake.

---

**Problem**: Nix installation fails

**Solution**: Check Nix installer action version and logs. Ensure GitHub runner has sufficient permissions.

---

**Problem**: Tests fail on CI but pass locally

**Solution**: Check environment differences. Verify Java version matches. Review CTest output in GitHub Actions logs.

### Test Failures

**Problem**: Generics test fails with "Type may not be parameterized"

**Solution**: Check that generic types are properly marked with `is_generic = true` when type parameters are added.

---

**Problem**: Assertion failure during test

**Solution**: Check semantic environment stack. Ensure `processing_type` is set during header processing.

## Success Criteria

A successful build:
- ✅ Compiles on Ubuntu using Nix + CMake
- ✅ Creates working `jikes` binary
- ✅ All CTest tests pass (36 tests: 22 compile + 14 runtime)
- ✅ Java 5 feature matrix tests all pass

## Future Enhancements

### Planned Improvements

1. **Code Coverage**
   - Add coverage reporting (gcov/lcov)
   - Upload to Codecov or Coveralls

2. **Additional Platforms**
   - macOS support via Nix
   - Additional Linux distributions

3. **Performance Benchmarks**
   - Track compilation speed
   - Compare against javac

4. **Release Automation**
   - Automatic versioning
   - Binary releases on tags
   - Changelog generation

### Test Suite Status

All Java 5 features are now tested via the feature matrix job:

- ✅ **Generics** - `GenericTest.java`
- ✅ **Enums** - `ColorTest.java`
- ✅ **Varargs** - `VarargsTest.java`
- ✅ **Enhanced For** - `ForEachTest.java`
- ✅ **Autoboxing** - `BasicBoxingTest.java`
- ✅ **Static Imports** - `StaticFieldOnlyTest.java`

## Maintenance

### Updating Workflow

To modify the workflow:

1. Edit `.github/workflows/ci.yml`
2. Test locally with Nix first
3. Commit and push
4. Monitor Actions tab for results

### Dependency Updates

Dependencies are managed via Nix flake:

- Update `flake.nix` for dependency changes
- Run `nix flake update` to update lock file
- GitHub Actions uses the locked dependencies
- Actions: Update `@v14` to newer versions as released

## Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Workflow Syntax](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions)
- [Available Runners](https://docs.github.com/en/actions/using-github-hosted-runners/about-github-hosted-runners)

---

**Last Updated**: 2025-11-22
**Maintainer**: Jikes Java 5 Modernization Project
