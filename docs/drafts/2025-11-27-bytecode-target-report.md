Bytecode target compatibility quick report
==========================================

Context
- Goal: ensure emitted classfiles respect requested -target level (no features from newer JVMs).
- Scope reviewed: codegen pipeline around classfile versioning, attributes, and StackMapTable generation in `src/codegen/bytecode.cpp`, plus option parsing in `src/option.cpp`.

Findings
- Classfile version is set per target: SDK1_1..SDK1_8 map to 45.3..52.0 at `src/codegen/bytecode.cpp:8008-8041`.
- StackMapTable is only generated for `target >= SDK1_7` (init and emit guarded in `BeginMethod`/`EndMethod` around lines 950-1118), so 1.6 and below will not get StackMap/StackMapTable.
- Synthetic handling is version-aware: for `target < SDK1_5`, the Synthetic attribute is explicitly added to fields/methods/classes (`src/codegen/bytecode.cpp` around lines 507-511, 8712-8716).
- Parameter annotations metadata is never emitted anywhere, so we cannot accidentally add it for pre-1.5 (but also cannot produce it for 1.5+).
- Generics `Signature` attributes are always emitted when generics are present (`src/codegen/bytecode.cpp` around lines 529-541 and 8723-8733) without checking target. A mismatched `-source 1.5` with `-target 1.4` would still attach `Signature` to a 48.0 classfile, which is outside the 1.4 spec. No guard prevents this combination.
- Other pre-5 shims remain conditioned on `target < SDK1_5` (e.g., class-literal helpers, multidimensional null checkcasts).

Residual risk / suggestions
- Add a target guard before emitting `Signature` (and any other Java 5+ metadata) or refuse incompatible `-source/-target` pairs to avoid spec violations for 1.4/1.3 outputs.
- If needed, add explicit tests that inspect produced classfile attributes per target (even with modern `java`), asserting absence/presence of StackMapTable, Signature, and Synthetic as per target rules.
