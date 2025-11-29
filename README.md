# JOPA: Javac One Patch Away

[![CI](https://github.com/7mind/jopa/actions/workflows/ci.yml/badge.svg)](https://github.com/7mind/jopa/actions/workflows/ci.yml)
[![License: IBM-PL 1.0](https://img.shields.io/github/license/7mind/jopa)](LICENSE)
[![Built with Nix](https://img.shields.io/badge/built%20with-nix-5277C3?logo=nixos&logoColor=white)](https://nixos.org/)

A totally Claude'd effort in modernizing [`jikes`](https://github.com/daveshields/jikes), the historical independent `javac` implementation in C++.

Fully supports Java 5, 6 and 7 both in syntax and bytecode. Can emit older bytecode versions for newer syntax (e.g. Java 5 bytecode for Java 7 programs).
Java 8 support is limited to default methods; other Java 8 features are intentionally not implemented.

Could be useful for [bootstrap](https://bootstrappable.org/) purposes.

How many bugs are here? Plenty. Currently we have 200+ end-to-end tests which run real program on real Hotspot JVM without `noverify`.
Also we partially check for JDK [compliance](#jdk-compliance-snapshot), but the parser, the typer and the bytecode generator are definitely buggy.
The original compiler had many bugs too.

## DevJopaK: No-Blob Bootstrap Java Development Kit

JOPA can build a fully self-contained Java development kit without any prebuilt binary blobs in the chain:

- **JOPA** (C++) - Java compiler, built from source
- **JamVM** (C) - Java Virtual Machine, built from source
- **GNU Classpath** - Java runtime library, **compiled by JOPA itself**
- **JamVM classes** - Bootstrap classes for JamVM, **compiled by JOPA itself**

This creates a complete Java toolchain where all Java bytecode is compiled from source using JOPA, making it suitable for reproducible and auditable builds.

## Motivation

I wanted to better understand capabilites and applicability ceilings of the latest generation of models (Opus 4.5, Sonnet 4.5, gpt-5.1-codex-max, Gemini 2.5 Pro) 
and learn how can I use them better. When I tasked Claude to refresh Jikes, I dind't expect it to succeed at all. It outperformed my expectations.

Good things I've learned:

- Today (in 2025) models can do much more than 2 years ago. In 2023 I hoped that I could get a single compilation unit done with an agent, now I can think about compilers.
- Models can help us revitalize ancient codebases which humans cannot handle
- Models can work with C++ and produce code which doesn't fail on any sanitizers, including memory leak sanitizers.
- The best language for the models is Python, they deliver much faster when they work with Python codebase.
- Models can debug, read logs and use all the utilities in your toolchain with extremely high efficiency. They are much more knowledgeable than any engineer out there and sometimes they can happily debug Java bytecode by hexdumps.
- You can work with codebases consisting of tens of thousands of lines. Even C++ lines.

Bad things:

- Models cannot abstract well and cannot generalize well. They are like humans and tend to deliver solutions for specific problems they see, but they generalize much less.
- Model outputs may look correct individually but not compose at all. Again, they cannot generalize.
- When unsupervised, they fail spectacularly in large refactorings and cannot design at all (again, incapable of generalization). I've tried to modularize this compiler, decouple components, replace the parser, I've tried to do many other transformations, all that failed, Claude is incapable of deep thinking and planning.
- They tend to take any shortcuts possible and behave like a bad genie.
- Codex and Gemini are MUCH less capable, on projects of this scale (~50000 C++ lines) they cannot produce coherent output at all. Claude is MUCH better. But again, on codebases of this size you cannot perform global tasks with Claude.
- Claude can easily get sidetracked and forget main goal
- Claude's CLI tool has insane memory leaks, the experience is very painful
- Frequently, Claude cannot see "obvious" solutions
- Claude loves to tell you that something alike to "we did a lot, let's make a coffee break". It's hard to make it work in a loop until it delivers.
- Codex and Geminin cannot work in a loop at all. Despite all the effort, they stop fast.
- You have to be always in the loop (more on that below). You cannot leave them unsupervised - they won't deliver.
- Models cannot concentrate on guidelines long enough.
- The models may mess up, delete files, overwrite files and do whatever random shit you can imagine. Don't trust them, isolate them. Commit often, be ready to reset environments.
- Claude cannot implement hard things. Even if it sees the logic of StackMap frame generation in OpenJDK - it cannot generalize it and reproduce here, it did amazing job but the implementation is still failing on many test cases.

Usage patterns:

- Be in the loop, monitor what the agent does, think and steer it towards the goal.
- Record important patters into your [CLAUDE.md](./CLAUDE.md). Try to be precise, 
- It might help to setup one model (Codex in my case) as a reviewer, so it would steer another (Claude) towards the goal. Unfortunately, there is no nice and convenient tool to organize such setup (there are some though), so at this point in time it's a good idea to be creative and write (vibe-code) your own.
- Always ensure that your model is working in reproducible environment (like Nix), instruct it to use your environment and tooling, start by writing tests and implementing reproducible builds. Toolings and environments are extremely important for model efficiency. Make your tests and builds fast. Apply TDD. Insist that the model should always run tests and ensure they all pass.
- Run models in [FireJail](https://github.com/netblue30/firejail) without their own sandboxes, that's the only way to be at least a bit productive.

And also:

- Java is complicated, there are a lot of things which are simple as concepts but require meticulous attention to be implemented properly (autoboxing, generics/type inference, reflection metadata, stack maps, synthetic methods, bridges, varargs, ...). The mere fact that Claude was able to implement such high level of compliance is impressive.
- It's hard to test Java compilers - you have comprehensive testkits but they are badly organized and hard to use. The worst thing is that there is no separate validation tool - you cannot properly validate your bytecode w/o running it under JVM. You cannot validate that your bytecode is compliant with particular bytecode version without  You need a JVM of a particular version to validate, and these JVMs might be hard to source, install, etc.
- In terms of compiler development and the development culture - we live in a much better world than before. We don't need to optimize compilers for memory and performance, we can use nice parser and multiple versions of immutable syntax trees, one per phase, we can run complex recursive alghoritms and we do write tests - previous generations used to live in a much less pleasant environment, the original compiler is a mess if we look at it from 2025.
- It's funny and sad, that GNU Classpath, Jikes and other ancient tools like JamVM are the only way to bootstrap OpenJDK in our days. If these small project didn't exist, the bootstrap won't be possible at all.
- It's sad that GNU and hacker culture is dying. Most of the stuff we do isn't fun at all.

### Building DevJopaK

```bash
cmake -B build -S . -DJOPA_BUILD_JAMVM=ON
cmake --build build --target devjopak
```

This creates `devjopak-<version>.tar.gz` containing:
- `bin/javac` - JOPA compiler wrapper
- `bin/java` - JamVM runtime wrapper
- `lib/jopa` - JOPA compiler binary
- `lib/jamvm` - JamVM runtime binary
- `lib/classes.zip` - JamVM classes (compiled by JOPA)
- `lib/glibj.zip` - GNU Classpath runtime
- `lib/native/` - GNU Classpath native libraries

### Using DevJopaK

```bash
tar xzf devjopak-*.tar.gz
./devjopak/bin/javac Hello.java
./devjopak/bin/java Hello
```

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
| JDK 8       | 4501               | 4222           | 93.8%            | 1428              | 744           | 52.1%           | N/A              | N/A                |
| JDK 7       | 3029               | 2983           | 98.5%            | 849               | 568           | 66.9%           | N/A              | N/A                |

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
| `jamvm_with_gnucp` | Bootstrap JamVM + GNU Classpath | `build/vendor-install/` |
| `devjopak` | Distribution archive | `build/devjopak-<version>.tar.gz` |

```bash
cmake --build build                              # Default: compiler + runtime
cmake --build build --target jopa                # Compiler only
cmake --build build --target jamvm_with_gnucp    # Bootstrap (compiles GNU Classpath with JOPA)
cmake --build build --target devjopak            # Create distribution archive
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
| `JOPA_BUILD_JAMVM` | ON | Enable JamVM + GNU Classpath bootstrap targets |
| `JOPA_ENABLE_JVM_TESTS` | ON | Enable runtime validation tests |
| `JOPA_USE_JAMVM_TESTS` | OFF | Run tests with JamVM instead of system JVM |
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
