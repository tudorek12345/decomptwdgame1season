# Public repository instructions

This is the source/tooling/progress publication of a larger local research project. Keep game binaries, extracted assets, bulk recovered listings, saves, third-party tools and captured machine-code oracle headers out of this repository and its history.

The complete source-only game remains unfinished. Keep reference export, compilable reconstruction, behavioral comparison and tested runtime claims separate. Progress percentages must state their denominator. Fourteen reconstructed functions out of 18,299 recognized functions is an unweighted count, not a measure of remaining engineering effort.

Build with CMake and run the public source-contract test. `TWD_BUILD_CAPTURE_TESTS` is off in a public checkout because the oracle headers are absent. Do not add fake fixtures, stubs or no-op replacements to make tests or game completion appear successful.

The reports are measurements from the local research build. A new compilation does not automatically inherit gameplay validation. Match EXE and DLL hashes before associating runtime evidence with a changed build.

Read `docs/PROGRESS.md`, `docs/ROADMAP.md` and `docs/LOCAL_RESEARCH.md` before expanding scope. Treat unmeasured tasks and indirect dependencies as unresolved. No calendar date is committed for a complete engine or port.
