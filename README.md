# JOPA: Java Object Program Assembler

A totally Claude'd effort in modernizing `jikes`, the historical independent `javac` implementation in C++.

Could be useful for [bootstrap](https://bootstrappable.org/) purposes.

## Java 5 Support

This fork adds comprehensive Java 5 (J2SE 5.0) language features:

- ✅ **Generics** - Type erasure with generic classes, methods, and bounded type parameters
- ✅ **Enhanced For-Loop** - For-each loops for arrays and Iterable collections
- ✅ **Varargs** - Variable-length argument lists with automatic array creation
- ✅ **Enums** - Enumerated types with synthetic methods (values(), valueOf())
- ✅ **Autoboxing/Unboxing** - Automatic conversions between primitives and wrappers (assignments, method args, return values, arithmetic)
- ✅ **Static Imports** - Import static members (single field, single method, wildcard)

**Test Coverage:** 36 tests (22 compile + 14 runtime) - all passing

## Building

### CMake (Recommended)

```bash
# With Nix
nix develop
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DJIKES_ENABLE_SOURCE_15=ON
cmake --build build -j$(nproc)

# Run tests
cd build && ctest --output-on-failure

# Install
cmake --install build --prefix /usr/local
```

### Legacy Autoconf/Make

```bash
nix develop
./configure --enable-source15
make -j$(nproc)
make check
```


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
