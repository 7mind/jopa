# CI/CD Setup for Jikes

## Overview

This repository uses GitHub Actions for continuous integration and testing. The workflow automatically builds Jikes and runs the Java 5 generics test suite on every push and pull request.

## Workflow: Build and Test

**File**: `.github/workflows/build-and-test.yml`

### Triggers

- **Push** to branches: `main`, `master`, `develop`
- **Pull Requests** to branches: `main`, `master`, `develop`

### Build Matrix

Tests run on multiple platforms:
- **Ubuntu Latest** (Linux)
- **macOS Latest**

### Workflow Steps

1. **Checkout Repository**
   - Uses `actions/checkout@v3`

2. **Install Dependencies**
   - **Ubuntu**: `build-essential`, `autoconf`, `automake`, `libtool`
   - **macOS**: `autoconf`, `automake`, `libtool` via Homebrew

3. **Configure Build**
   - Runs `autoreconf -fi` if needed
   - Executes `./configure`

4. **Build Jikes**
   - Compiles the compiler in `src/` directory
   - Creates `src/jikes` binary

5. **Verify Binary**
   - Checks that `jikes` binary was created
   - Displays version/help information

6. **Run Generics Tests**
   - Tests simple generic classes
   - Tests multiple type parameters
   - Tests bounded type parameters
   - Runs batch compilation test

7. **Upload Artifacts**
   - Uploads compiled `jikes` binary
   - Uploads test results and logs
   - Available for 90 days

## Local Testing

### Quick Test
```bash
cd test-generics
./run-tests.sh
```

### Manual Testing
```bash
# Build Jikes
cd src
make

# Run individual test
cd ../test-generics
../src/jikes -sourcepath runtime -d . SimpleGeneric.java

# Verify bytecode
ls -la *.class
od -c SimpleGeneric.class | grep Signature
```

## Test Suite

### Current Tests

1. **SimpleGeneric.java** - Basic generic class with one type parameter
2. **TwoTypeParams.java** - Generic class with multiple type parameters
3. **BoundedGeneric.java** - Generic class with bounded type parameter

### Adding New Tests

1. Create test file in `test-generics/`
2. Add test case to `run-tests.sh`
3. Add test step to `.github/workflows/build-and-test.yml`
4. Commit and push

Example:
```bash
# Add to run-tests.sh
run_test "My New Feature" "MyNewTest.java"

# Add to workflow YAML
- name: Test my new feature
  run: |
    cd test-generics
    ../src/jikes -sourcepath runtime -d . MyNewTest.java
    test -f MyNewTest.class && echo "✓ MyNewTest compiled successfully"
```

## Workflow Status

You can view the workflow status:
- On GitHub: Repository → Actions tab
- Badge in README (to be added)

## Troubleshooting

### Build Failures

**Problem**: `configure` script not found

**Solution**: Workflow runs `autoreconf -fi` automatically. If this fails, ensure `configure.ac` or `configure.in` exists.

---

**Problem**: Missing dependencies

**Solution**: Update the dependency installation steps in the workflow for your platform.

---

**Problem**: Tests fail on one platform but not another

**Solution**: Check platform-specific behavior. May need conditional compilation or platform-specific test cases.

### Test Failures

**Problem**: Generics test fails with "Type may not be parameterized"

**Solution**: Check that generic types are properly marked with `is_generic = true` when type parameters are added.

---

**Problem**: Assertion failure during test

**Solution**: Check semantic environment stack. Ensure `processing_type` is set during header processing.

## Success Criteria

A successful build:
- ✅ Compiles on Ubuntu and macOS
- ✅ Creates working `jikes` binary
- ✅ All generics tests pass
- ✅ At least 3 test classes compile successfully

## Future Enhancements

### Planned Improvements

1. **Code Coverage**
   - Add coverage reporting (gcov/lcov)
   - Upload to Codecov or Coveralls

2. **Additional Platforms**
   - Windows (MinGW or Cygwin)
   - Additional Linux distributions

3. **Performance Benchmarks**
   - Track compilation speed
   - Compare against javac

4. **Release Automation**
   - Automatic versioning
   - Binary releases on tags
   - Changelog generation

### Test Suite Expansion

As Phase 2 features are implemented:

1. **Enhanced For-Loop Tests**
   ```yaml
   - name: Test enhanced for-loop
     run: |
       cd test-generics
       ../src/jikes -sourcepath runtime -d . ForEachTest.java
   ```

2. **Varargs Tests**
   ```yaml
   - name: Test varargs
     run: |
       cd test-generics
       ../src/jikes -sourcepath runtime -d . VarargsTest.java
   ```

3. **Enum Tests**
   ```yaml
   - name: Test enums
     run: |
       cd test-generics
       ../src/jikes -sourcepath runtime -d . EnumTest.java
   ```

## Maintenance

### Updating Workflow

To modify the workflow:

1. Edit `.github/workflows/build-and-test.yml`
2. Test locally first if possible
3. Commit and push
4. Monitor Actions tab for results

### Dependency Updates

Dependencies are installed fresh on each run. To update versions:

- **Ubuntu**: Packages come from apt repositories
- **macOS**: Homebrew formulas auto-update
- **Actions**: Update `@v3` to `@v4` etc. as new versions release

## Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [Workflow Syntax](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions)
- [Available Runners](https://docs.github.com/en/actions/using-github-hosted-runners/about-github-hosted-runners)

---

**Last Updated**: 2025-11-22
**Maintainer**: Jikes Java 5 Modernization Project
