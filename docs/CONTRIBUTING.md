# Contributing to Jikes Java 5 Modernization

Thank you for your interest in contributing to the Jikes Java 5 modernization project!

## Project Overview

This project aims to modernize the ancient Jikes Java compiler to support Java 5 (JDK 1.5) language features including:

- ✅ **Generics** (Complete)
- ⏳ Enhanced for-loop
- ⏳ Varargs
- ⏳ Static imports
- ⏳ Enums
- ⏳ Annotations

## Getting Started

### Prerequisites

- **C++ Compiler**: GCC or Clang
- **Build Tools**: CMake 3.15+
- **Recommended**: Nix with direnv for reproducible builds

### Building Jikes

```bash
# Clone the repository
git clone <repository-url>
cd jikes

# Configure and build with CMake
nix develop  # or ensure cmake is available
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJIKES_ENABLE_SOURCE_15=ON
cmake --build build -j$(nproc)

# Verify build
./build/src/jikes -version
```

### Running Tests

```bash
# Quick test
cd test-generics
./run-tests.sh

# Manual test
cd test-generics
../src/jikes -sourcepath runtime -d . SimpleGeneric.java
```

## Development Workflow

### 1. Find or Create an Issue

Check the [issue tracker](link) for open issues or create a new one describing:
- The feature you want to implement
- The bug you want to fix
- Your proposed approach

### 2. Create a Branch

```bash
git checkout -b feature/your-feature-name
# or
git checkout -b fix/your-bug-fix
```

### 3. Make Changes

Follow these guidelines:
- **Code Style**: Match existing code style in the file
- **Comments**: Add comments for non-obvious logic
- **Documentation**: Update relevant docs
- **Tests**: Add tests for new features

### 4. Test Your Changes

```bash
# Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJIKES_ENABLE_SOURCE_15=ON
cmake --build build -j$(nproc)

# Run all tests
cd build
ctest --output-on-failure

# Add your own tests
# Create YourTest.java in test-generics/
# Add to CMakeLists.txt
```

### 5. Commit

Write clear commit messages:

```
Add enhanced for-loop support

- Implement parser changes for for-each syntax
- Add semantic analysis for Iterable/array detection
- Generate desugared bytecode
- Add tests for arrays and collections

Fixes #123
```

### 6. Submit Pull Request

- Push your branch to GitHub
- Create a Pull Request with clear description
- Link to related issues
- Wait for CI to pass
- Respond to review feedback

## Code Structure

### Key Directories

- `src/` - Compiler source code
  - `parser.g` - Grammar definition
  - `ast.h` - AST node definitions
  - `decl.cpp` - Declaration processing
  - `body.cpp` - Statement/expression processing
  - `bytecode.cpp` - Bytecode generation
  - `symbol.h/cpp` - Symbol table

- `test-generics/` - Test files for Java 5 features
- `.github/` - CI/CD configuration

### Important Files

- **Generics Implementation**:
  - `src/typeparam.h/cpp` - Type parameters
  - `src/paramtype.h/cpp` - Parameterized types
  - Added ~2,500 lines total

- **Symbol Table**:
  - `src/symbol.h` - Type, method, variable symbols
  - Extended for generic support

- **Semantic Analysis**:
  - `src/decl.cpp` - Type processing
  - `src/body.cpp` - Method/statement processing

## Coding Guidelines

### C++ Style

```cpp
// Use existing naming conventions
class TypeParameterSymbol { ... }  // PascalCase for classes
void ProcessTypeParameters(...);    // PascalCase for methods
TypeSymbol* type_symbol;           // snake_case for variables

// Pointer/reference style
TypeSymbol* symbol;  // Pointer on type side
TypeSymbol& symbol;  // Reference on type side

// Comments
// Brief description
void SomeMethod() {
    // Implementation details
}
```

### Error Handling

```cpp
// Report semantic errors clearly
ReportSemError(SemanticError::ERROR_CODE,
               token_location,
               "descriptive message");

// Validate early
if (!valid_condition) {
    // Report error and return early
    return;
}
```

### Testing

For each feature:

1. **Create test files** in `test-generics/`
2. **Add to test script** (`run-tests.sh`)
3. **Add to CI workflow** (`.github/workflows/build-and-test.yml`)
4. **Document expected behavior**

Example test file:
```java
// TestFeature.java
public class TestFeature {
    // Test your feature here
}
```

## Documentation

### Code Comments

- Add comments for complex algorithms
- Explain "why" not "what"
- Reference JLS sections when implementing spec features

### Documentation Updates

When adding features:
- Update `JAVA5_IMPLEMENTATION_PLAN.md`
- Add test results to `test-generics/TEST_RESULTS_FINAL.md`
- Update this CONTRIBUTING guide if needed

## Current Priorities

See `JAVA5_IMPLEMENTATION_PLAN.md` for detailed roadmap.

**Next up**:
1. Enhanced for-loop (2-3 hours)
2. Varargs (3-4 hours)
3. Static imports (3-4 hours)

## Getting Help

- **Questions**: Open a discussion
- **Bugs**: Create an issue
- **Design decisions**: Reference the implementation plan

## Code Review Process

All contributions go through code review:

1. **CI must pass** - All tests green
2. **Code review** - At least one approval
3. **Documentation** - Updated as needed
4. **Tests** - New features must have tests

## License

By contributing, you agree that your contributions will be licensed under the same license as the project.

## Recognition

Contributors will be acknowledged in:
- Git commit history
- Release notes
- Project documentation

---

Thank you for contributing to Jikes! 🚀
