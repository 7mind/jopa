# JOPA: Javac One Patch Away

[![CI](https://github.com/7mind/jopa/actions/workflows/ci.yml/badge.svg)](https://github.com/7mind/jopa/actions/workflows/ci.yml)
[![License: IBM-PL 1.0](https://img.shields.io/github/license/7mind/jopa)](LICENSE)
[![Built with Nix](https://img.shields.io/badge/built%20with-nix-5277C3?logo=nixos&logoColor=white)](https://nixos.org/)

A totally Claude'd effort in modernizing [`jikes`](https://github.com/daveshields/jikes), the historical independent `javac` implementation in C++.

Fully supports Java 5, 6 and 7 both in syntax and bytecode. Can emit older bytecode versions for newer syntax (e.g. Java 5 bytecode for Java 7 programs).
Java 8 support is limited to default methods; other Java 8 features are intentionally not implemented.

Could be useful for [bootstrap](https://bootstrappable.org/) purposes.

How many bugs are here? Plenty. Currently we have 200+ end-to-end tests which run real programs compiled with JOPA on real Hotspot JVM without `noverify`.
Also we partially check for JDK [compliance](#jdk-compliance-snapshot), but the parser, the typer and the bytecode generator are definitely buggy.
The original compiler had many bugs too.

JOPA has a flaw: its memory consumption might be quiet high (tens of GiBs of RAM) due to the way the generics are implemented.

## Achievement Unlocked: Bootstrapping ECJ and Ant

JOPA has successfully achieved a significant milestone in bootstrapping:

-   **Eclipse Compiler for Java (ECJ) 4.2.1**
-   **Apache Ant 1.8.4**

Both are built from source using JOPA and run on **JamVM 2.0.0** with **GNU Classpath 0.99**. This demonstrates that JOPA is a capable and viable stage in a full-source bootstrap chain, bridging the gap between C++ and a fully featured Java 7 compiler environment.

Previously, bootstrapping these tools required a much longer chain or binary blobs. JOPA simplifies this by providing a C++ compiler that can directly build these complex Java applications.

## DevJopaK: No-Blob Bootstrap Java Development Kit

JOPA can build a fully self-contained Java development kit without any prebuilt binary blobs in the chain:

- **JOPA** (C++) - Java compiler, built from source
- **JamVM** (C) - Java Virtual Machine, built from source
- **GNU Classpath** - Java runtime library, **compiled by JOPA itself**
- **JamVM classes** - Bootstrap classes for JamVM, **compiled by JOPA itself**

This creates a complete Java toolchain where all Java bytecode is compiled from source using JOPA, making it suitable for reproducible and auditable builds.

[Notes on this project, LLMs and the lessons](EXPLANATION.md).

### Building DevJopaK

DevJopaK is built as a separate CMake project that depends on the main JOPA build.

1. Build JOPA (compiler and stub runtime):
```bash
cmake -B build
cmake --build build
```

2. Build DevJopaK (JDK distribution):
```bash
cmake -S devjopak -B build-devjopak -DJOPA_BUILD_DIR=build
cmake --build build-devjopak --target devjopak
```

This creates `build-devjopak/devjopak-<version>.tar.gz` containing:
*   `bin/javac` (JOPA wrapper)
*   `bin/java` (JamVM wrapper)
*   `bin/ant` (Apache Ant wrapper)
*   `lib/jopa` (compiler binary)
*   `lib/jamvm` (runtime binary)
*   `lib/glibj.zip` (GNU Classpath library)
*   `lib/classes.zip` (JamVM bootstrap classes)
*   `lib/ant.jar` (Apache Ant libraries)

#### DevJopaK CMake Build Details

The `devjopak` CMake project (`devjopak/CMakeLists.txt`) provides specific targets and options for building the full distribution.

**Configuration Options:**

| Option | Default | Description |
|--------|---------|-------------|
| `JOPA_BUILD_DIR` | `../build` | Path to the main JOPA compiler build directory. |
| `JOPA_CLASSPATH_VERSION` | `0.99` | GNU Classpath version to use (`0.93` or `0.99`). |

**Build Targets:**

| Target | Description | Output |
|--------|-------------|--------|
| `gnu_classpath` | Builds the GNU Classpath library using JOPA. | `build-devjopak/vendor-install/classpath/` |
| `jamvm_with_gnucp` | Builds JamVM with GNU Classpath. | `build-devjopak/vendor-install/jamvm/` |
| `apache_ant` | Builds Apache Ant using JOPA/JamVM. | `build-devjopak/vendor-install/ant/` |
| `devjopak` | Creates the full DevJopaK distribution archive. | `build-devjopak/devjopak-<version>.tar.gz` |
| `devjopak-ecj` | Creates the DevJopaK distribution with ECJ as the compiler. | `build-devjopak/devjopak-ecj-<version>.tar.gz` |

#### Legacy Classpath Testing

You can select the GNU Classpath version using the `JOPA_CLASSPATH_VERSION` option when building DevJopaK:

```bash
cmake -S devjopak -B build-devjopak -DJOPA_BUILD_DIR=build -DJOPA_CLASSPATH_VERSION=0.93
cmake --build build-devjopak --target gnu_classpath
```

**Note:** Setting version to `0.93` disables JamVM and the `devjopak` target, as JamVM 2.0.0 requires a newer class library. This mode is primarily for historical compatibility testing of the compiler itself.

### Using DevJopaK

```bash
tar xzf devjopak-*.tar.gz
./devjopak/bin/javac Hello.java
./devjopak/bin/java Hello
```

### Using DevJopaK-ECJ

The `devjopak-ecj` distribution works identically but uses the Eclipse Compiler for Java (ECJ) instead of JOPA.

```bash
tar xzf devjopak-ecj-*.tar.gz
./devjopak-ecj/bin/javac Hello.java
./devjopak-ecj/bin/java Hello
```

### Validation Script

To quickly verify a DevJopaK installation, use the provided validation script:

```bash
tar xzf devjopak-*.tar.gz
./devjopak/bin/devjopak-validate
```
This script will compile and run a simple "Hello, DevJopaK!" program and check the installed Java and Javac versions.

## Relevant projects:

- Sister project, Java compiler in Python: [PyJOPA](https://github.com/7mind/pyjopa)
- JVM implementation in Python: [python-jvm-interpreter](https://github.com/gkbrk/python-jvm-interpreter)
- JVM implementation in Common Lisp: [OpenLDK](https://github.com/atgreen/openldk)

Note: I've made multiple attempts to replace the legacy parser with a more modern one but Claude failed to deliver due to extremely tight coupling.

## Java 5, 6, 7 Support

This fork adds comprehensive Java 5 (J2SE 5.0), Java 6 (Java SE 6), and Java 7 (Java SE 7) language features:

### Java 5 Features
- ✅ **Generics** - Type erasure with generic classes, methods, and bounded type parameters
- ✅ **Enhanced For-Loop** - For-each loops for arrays and Iterable collections
- ✅ **Varargs** - Variable-length argument lists with automatic array creation
- ✅ **Enums** - Enumerated types with synthetic methods (values(), valueOf())
- ✅ **Autoboxing/Unboxing** - Automatic conversions between primitives and wrappers (assignments, method args, return values, arithmetic)
- ✅ **Static Imports** - Import static members (single field, single method, wildcard)
- ✅ **Annotations** - Marker, single-element, and full annotations

### Java 6 Features
- ✅ **Class file version 50.0** - Generate Java 6 bytecode with `-target 1.6`
- ✅ **Debug information** - Enhanced debugging with `-g` flag for parameter names and local variables

### Java 7 Features
- ✅ **Diamond Operator** - Type inference for generic instance creation (`new ArrayList<>()`)
- ✅ **Multi-catch** - Catching multiple exception types in a single catch block (`catch (IOException | SQLException e)`)
- ✅ **Try-with-resources** - Automatic resource management with `AutoCloseable` interface and exception suppression via `addSuppressed()`
- ✅ **Strings in Switch** - Switch statements with String expressions
- ✅ **Binary Literals** - Integer literals in binary form (`0b1010`)
- ✅ **Underscores in Numeric Literals** - Improved readability (`1_000_000`)

### Java 7 Bytecode Status
Java 7 language features are fully supported for parsing, semantic analysis, and bytecode generation:

| Feature | `-target 1.5` | `-target 1.6` | `-target 1.7` |
|---------|---------------|---------------|---------------|
| Diamond operator | ✅ Works | ✅ Works | ✅ Works |
| Multi-catch | ✅ Works | ✅ Works | ✅ Works |
| Try-with-resources | ✅ Works | ✅ Works | ✅ Works |
| String switch | ✅ Works | ✅ Works | ✅ Works |
| Exception suppression | ✅ Works | ✅ Works | ✅ Works |
| Binary/underscore literals | ✅ Works | ✅ Works | ✅ Works |

**Note:** Targets 1.5 and 1.6 pass the full test suite with strict JVM verification. Target 1.7 (class version 51.0) has known StackMapTable limitations with complex boolean expressions used as method arguments (e.g., `test("name", a == b)`). For most code, target 1.7 works correctly; alternatively, use `-target 1.5` or `-target 1.6` for maximum compatibility. Generated class files require at least the corresponding JVM major version.

### JDK Compliance Snapshot

| JDK Version | Whitelisted parser | Passing parser | Passing parser % | Whitelisted typer | Passing Typer | Passing Typer % | Passing bytecode | Passing bytecode % |
|-------------|--------------------|----------------|------------------|-------------------|---------------|-----------------|------------------|--------------------|
| JDK 8       | 4501               | 4222           | 93.8%            | 1428              | 834           | 58.4%           | N/A              | N/A                |
| JDK 7       | 3029               | 2983           | 98.5%            | 849               | 671           | 79.0%           | N/A              | N/A                |

Bytecode test columns are `N/A` because we currently only compile the JDK tests via `scripts/test-java8-compliance.sh` / `scripts/test-java7-compliance.sh` and do not execute a separate bytecode validation suite yet.

### Advanced Generics Support
The compiler fully supports complex generic type signatures including:
- ✅ **Type Token Pattern** - Gafter's pattern for capturing generic types at runtime
- ✅ **Nested Parameterized Types** - `List<Map<String, List<Integer>>>`
- ✅ **Bridge Methods** - Automatic generation for covariant returns and generic overrides
- ✅ **Signature Attributes** - Full JVM signature generation for reflection support

### Java 8 Features (Partial)
- ✅ **Default Methods** - Interface methods with default implementations
- ❌ **Static Methods in Interfaces** - Requires class file version 52.0
- ❌ **Lambda Expressions** - Not implemented
- ❌ **Method References** - Not implemented

## Runtime Compatibility
- `-target 1.5` → class file version 49.0 (runs on Java 5+)
- `-target 1.6` → class file version 50.0 (runs on Java 6+)
- `-target 1.7` → class file version 51.0 with StackMapTable (runs on Java 7+)

## Building

### Requirements
- CMake 3.20+ and a C++17 compiler (Clang recommended)
- libzip
- iconv and/or ICU (uc) for encoding support
- Java JDK (only for running tests - not needed for compilation)
- Optional: Nix/direnv for reproducible environment

### Quick Start

```bash
# With Nix (recommended)
nix develop
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure

# Generic CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix /usr/local
```

### Build Targets

| Target | Description | Output |
|--------|-------------|--------|
| *(default)* | Compiler + stub runtime | `build/src/jopa`, `build/runtime/jopa-stub-rt.jar` |
| `jopa` | Compiler only | `build/src/jopa` |
| `jopa-stub-rt` | Stub runtime JAR | `build/runtime/jopa-stub-rt.jar` |

```bash
cmake --build build                              # Default: compiler + runtime
cmake --build build --target jopa                # Compiler only
ctest --test-dir build                           # Run tests
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `JOPA_ENABLE_DEBUG` | OFF | Enable internal compiler debugging traces |
| `JOPA_ENABLE_NATIVE_FP` | ON | Use native floating point (vs emulation) |
| `JOPA_ENABLE_ENCODING` | ON | Enable `-encoding` support via iconv/ICU |
| `JOPA_ENABLE_SANITIZERS` | Debug builds | Enable ASan/UBSan |
| `JOPA_ENABLE_LEAK_SANITIZER` | OFF | Enable memory leak detection (requires `JOPA_ENABLE_SANITIZERS`) |
| `JOPA_ENABLE_JVM_TESTS` | ON | Enable runtime validation tests (uses system Java) |
| `JOPA_TARGET_VERSION` | 1.5 | Bytecode target version for tests (1.5, 1.6, 1.7) |

jikes
=====

Jikes - a Java source code to bytecode compiler - was written by Philippe Charles and Dave Shields of
IBM Research. 

Jikes was written from scratch, from August 1996 to its first release on IBM's alphaWorks site in
April 1997. Work continued until August, 1997, at which time the project was shut down so the authors could resume
full-time work on the addition of support for inner classes.

The next release of Jikes was in March, 1998.

The availability of this release spurred new interest in a Linux version. The release of a Linux binary version
in early July, 1998, set new single-day download records for IBM's alphaWorks site.

The availability of a Linux binary version was soon followed by requests for the source.  IBM management
approved the release in source form in September, 1998, followed by the release in early December, 1998.


Released in December, 1998, it was IBM's first open source project, and the first open-source project 
from IBMk to be included in a major Linux Distribution (Redhat, Fall 1999).

Jikes was notable both in its automatic error correction, in the quality of its error messages, and
its compilation speed. It was routinely 10-20 times faster than javac, the standard compiler for Java
when Jikes was released.

IBM's involvement in the Jikes project ended in late 1999. Work continued, first at IBM's Developerworks (where
IBM was the original project in the Open Source Zone), and later at Sourceforge.

Active work on the project ceased in 2005. Changes in the Java language, most notably in the introduction of generics,
made Jikes less attractive.

Jikes remains usable for beginners to the Java language, especially those interested in just the core
features of the language.

The authors also believe Jikes to be of interest for its compiler designed and implementation, an believe it
to be a suitable subject of study for an introductory compiler course.

Notable features include the following:

Written from scratch by two people. The only third-part code in the first version was used read Java binary
class files, where were in Zip format.

No use of parser construction tools other than the Jikes Parser Generator, written by Philippe Charles.

Written in C++.

Includes a very efficient storage allocator and memory management.

The present repository includes Jikes versions 1.04 through 1.22. (Jikes 1.00 through 1.03 seem to have lost).
The sources used were retrieved from the Sourcforge site in early July, 2012. Each version is identified by a git tag.

## Authors

- Originally written by Philippe Charles and David Shields of IBM Research.
- Subsequent contributors include, but are not limited to:
  - Chris Abbey
  - C. Scott Ananian
  - Musachy Barroso
  - Joe Berkovitz
  - Eric Blake
  - Norris Boyd
  - Ian P. Cardenas
  - Pascal Davoust
  - Mo DeJong
  - Chris Dennis
  - Alan Donovan
  - Michael Ernst
  - Bu FeiMing
  - Max Gilead
  - Adam Hawthorne
  - Diane Holt
  - Elliott Hughes
  - Andrew M. Inggs
  - C. Brian Jones
  - Marko Kreen
  - David Lum
  - Phil Norman
  - Takashi Okamoto
  - Emil Ong
  - Andrew Pimlott
  - Daniel Resare
  - Mark Richters
  - Kumaran Santhanam
  - Gregory Steuck
  - Brian Sullivan
  - Andrew G. Tereschenko
  - Russ Trotter
  - Andrew Vajoczki
  - Jerry Veldhuis
  - Dirk Weigenand
  - Vadim Zaliva
  - Henner Zeller
