![TWD / Source Recovery](docs/assets/source-recovery-banner.png)

# TWD · Source Recovery

C++ reconstruction research for **The Walking Dead: Season One**. The experimental hybrid runs; the standalone source engine is unfinished.

[MWCMP Discord](https://discord.gg/ufXqPcVdVj) · [Progress](docs/PROGRESS.md) · [Roadmap](docs/ROADMAP.md) · [Build notes](docs/LOCAL_RESEARCH.md)

## Status

| Track | Result |
| --- | ---: |
| Native reference exports | 18,299 / 18,299 recognized functions |
| Recovered Lua variants | 622, syntax-checked |
| Archive indexes | 134 / 134 |
| Native C++ replacements | **14 / 18,299 — 0.077%** |

The recognized-function export is complete. Source reconstruction is **0.077% by function count**, not by remaining effort. **18,285 recognized functions still depend on original machine code.**

## Verified

- **3,932,574** differential comparisons passed.
- **71.2 million** replacement-entry calls observed in the isolated hybrid.
- A save/reload preserved the test marker; **17,700** post-reload frames observed.
- Windows x86 and Linux source builds pass [CI](https://github.com/tudorek12345/decomptwdgame1season/actions/workflows/build.yml).

## Build

Windows: Visual Studio 2022 C++ tools and CMake 3.24+.

```powershell
cmake --preset vs2022-x86 -DTWD_BUILD_CAPTURE_TESTS=OFF
cmake --build --preset x86-release --parallel
ctest --preset x86-release --output-on-failure
```

This builds the replacement source and public contract tests. Game binaries, assets, bulk recovered listings and machine-code fixtures stay local.

## Next

String allocation and ownership: a proposed **16–40 focused-hour investigation**. Full engine completion has no reliable ETA; plan for a potentially multi-year solo project. [Timeline and acceptance criteria →](docs/ROADMAP.md)
