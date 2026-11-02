# Progress and measurement

Snapshot: **2 November 2026**. Baseline research commit: `9a6391f`.

The project has two separate tracks: exporting reference material from the installed build, and replacing original machine code with tested C++ source. A completed export is not a completed source engine.

| Measure | Result | Denominator and limit |
| --- | ---: | --- |
| Installed archives indexed | 134 / 134 — **100%** | Archives surveyed in this installation |
| Discovered Lua variants recovered and syntax-checked | 622 / 622 — **100%** | Includes 595 non-mod variants; syntax is not behavioral equivalence |
| Recognized native functions exported | 18,299 / 18,299 — **100%** | The current function catalog, not every possible function in the binary |
| Recognized function-body coverage | **84.085%** | 4,837,074 / 5,752,620 bytes of `.text`; includes no claim about correctness |
| Decoded instruction bytes | **88.538%** | 5,093,279 / 5,752,620 bytes of `.text` |
| Native functions replaced with source | 14 / 18,299 — **0.076507%** | Unweighted function count; displayed as **0.077%** |
| Whole-game completion | **Not measured** | No campaign-wide equivalence or workload-weighted denominator exists |

## What the percentages mean

The 18,299-function denominator includes engine and library routines. Functions differ greatly in size and complexity. Fourteen relatively small replacements cannot be used to infer the remaining development time. The denominator can also grow when new functions are identified.

There are still **18,285 recognized functions backed by original machine code**. The function catalog contains 5,060 listings with decompiler warnings. Even a warning-free listing can have incorrect types or missing semantics: correcting a falsely inferred `void` return restored a CRC64 calculation that the earlier listing had discarded.

The code-coverage percentages describe bytes classified by the saved analysis. Undecoded bytes may include data, padding, tables or missed code. They are not percentages of a working reimplementation.

## Reconstructed scope

| Subsystem | Functions | Contract |
| --- | ---: | --- |
| Geometry | 1 | Closest point on a segment; aliasing and degenerate NaNs |
| Hashing | 4 | CRC32, word hashing, CRC64 strings, ASCII-folded CRC64 strings |
| Symbols | 9 | Construction, copy, append, comparison and String32 selection |

The signed-buffer CRC64 routine at RVA `0x1eec10` remains unresolved. High bytes can index data preceding the lookup table, including relocated pointers. Substituting a conventional CRC implementation would change its behavior.

## Validation

| Experiment | Recorded result |
| --- | --- |
| Geometry against captured x86 routine | 100,032 comparisons in each static/DLL mode |
| Hash/symbols against captured x86 routines | 1,866,255 comparisons in each static/DLL mode |
| Combined differential comparisons | **3,932,574; zero failures** |
| Current hybrid runtime | **71,215,162** replacement-entry calls across seven entry points |
| Save/reload | Exact String marker preserved on a game agent |
| After reload | **17,700** observed scene frames |
| Bounded runtime test | 100 seconds; no unhandled exception |
| Direct-call audit | 111,240 calls; no target outside the recognized function catalog |

The runtime scene was `adv_streetoutskirts.scene`. Internal calls between reconstructed functions do not increment separate DLL-entry counters. Uncalled entry points are covered by the differential harness, not claimed as exercised during gameplay.

These results do not establish input-driven chapter completion, arbitrary save compatibility, a full campaign, or an x64 game port. The public source-contract test uses no game bytes; the differential oracles and runtime resources stay local.

Machine-readable counts: [progress.json](progress.json). Runtime identities: [acceptance record](../reports/current-runtime-acceptance.json). Analysis counts: [native coverage](../reports/native-coverage.json).
