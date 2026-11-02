# Public build and local research

The public repository contains reconstructed C++, analysis/patch tooling, test harness source, selected measurements and project artwork. It has a clean publication history. The full local research repository has a separate history containing recovered reference listings.

## Build from a public checkout

Use CMake 3.24 or later and Visual Studio 2022 with the C++ build tools for the Windows target:

```powershell
cmake --preset vs2022-x86 -DTWD_BUILD_CAPTURE_TESTS=OFF
cmake --build --preset x86-release --parallel
ctest --preset x86-release --output-on-failure
```

This builds the replacement library, x86 DLL, image utility and the public source-contract test. It does not produce a complete game executable or download game content.

The public VS Code build/test tasks perform the same steps. A public checkout cannot launch the game until the local runtime and its required inputs have been prepared.

For a portable library check:

```sh
cmake -S . -B build/portable -DCMAKE_BUILD_TYPE=Release -DTWD_BUILD_CAPTURE_TESTS=OFF
cmake --build build/portable --parallel
ctest --test-dir build/portable --output-on-failure
```

`String32` describes a 32-bit game layout. An x64 library build does not make its pointer fields a native x64 string representation and does not port the engine.

## Local-only inputs

The publication excludes `sources/`, `recovered/`, `native-analysis/`, `runtime/`, save files, game screenshots, third-party tool binaries and `tests/original_*.hpp` machine-code fixtures. It also excludes the local Git history that contains recovered listings.

The differential test source is published, but its oracles must be generated from the matching local analysis image. The Ghidra project and game-specific tool setup remain part of the local research environment.

The accepted original executable SHA-256 is:

```text
44a5ff21cbb085812c48179259141ff5c83eaeaa3423b4734b43533df0a2528d
```

The validated analysis image SHA-256 is:

```text
39df65b5e03e68703e04da90d516fbc15392706fbc9a4580f2aaf44ef78c33d9
```

The analysis image uses normalized `.text` from a prior memory capture and original disk globals/imports. Capture consistency was checked; the original capture PID and per-page read status were not preserved. Do not execute the analysis image directly.

`prepare_hash_fixture.py` generates the hash/symbol oracles from that exact image. `prepare_geometry_fixture.py` generates the geometry oracle. With both local headers present, enable `TWD_BUILD_CAPTURE_TESTS` and run the differential suites. The hybrid builder checks matching runtime input, expected original prefixes, DLL exports and fresh test results.

The optional Lua runtime fixture requests slot 1 and can run a marker save/reload in the isolated `TWD-Rebuild-01` namespace. Disable it after testing. Its logs describe an experiment, not a completed campaign playthrough.

## Reproducing the reported results

The public CI checks that the published source builds and its contract tests pass without game files. Historical original/replacement and gameplay measurements are linked in the README and tied to binary hashes. CI does not recreate those private-input experiments.

Selected public reports replace workstation-specific path prefixes with placeholders. Counts, RVAs, binary hashes and measured results remain unchanged. The runtime acceptance report points to the local evidence by its original relative filename; raw process logs and saves are not published.
