# JOPA TODO

## Known Issues

### Class file corruption with Comparable<T> + CharSequence in java.lang

When a class in the `java.lang` package implements both `Comparable<T>` and `CharSequence`,
and is compiled with `-sourcepath runtime`, the generated class file is corrupted.

**Reproduction:**
```bash
# This produces a corrupted class file:
./build/src/jopa -source 1.5 -sourcepath runtime -d /tmp/out runtime/java/lang/String.java
# With String.java implementing: java.io.Serializable, Comparable<String>, CharSequence
```

**Workaround:**
`java.lang.String` in stub runtime only implements `Serializable` and `CharSequence`, not `Comparable<String>`.

**Impact:**
- JDK tests that use `Comparable<String>` methods on String will fail
- Code that requires `String implements Comparable<String>` won't compile with stub runtime

**To investigate:**
- Look at bridge method generation when compiling java.lang classes
- The issue may be related to how the compiler handles self-referential generics in bootstrap mode
