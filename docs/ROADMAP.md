# Timeline and remaining work

As of **2 November 2026**, there is **no defensible completion date for a source-only game**. For solo planning, treat full engine reconstruction as a potentially multi-year research project. That is a low-confidence planning judgment, not an estimate derived from measured subsystem throughput.

The completed replacements are small geometry/hash/symbol routines. Rendering, animation, dialogue, resource ownership, allocators, scene state and indirect-call contracts have not been reconstructed as complete subsystems. Their integration cost is not represented by `14 / 18,299`, and extrapolating that fraction into a deadline would be misleading.

## Recorded milestones

| Snapshot | Milestone | Acceptance |
| --- | --- | --- |
| 2026-11-02 | Reference recovery | 18,299 recognized native exports; 622 script variants; 134 archive indexes |
| 2026-11-02 | Geometry reconstruction | Compiled source; 100,032 original/replacement comparisons per mode |
| 2026-11-02 | Hash/symbol reconstruction | 13 additional replacements; 1,866,255 comparisons per mode |
| 2026-11-02 | Hybrid runtime | Source entries exercised; opening scene running |
| 2026-11-02 | Save/reload | Serialized marker restored; continued post-load frames |

These are evidence snapshots. The earlier memory capture was inherited from prior work; its capture duration and original process provenance are not available.

## Proposed timeboxes

These are **work budgets for the next investigations**, not promised delivery times. They assume one person working focused engineering hours and access to the current local inputs and tools. No calendar dates are committed.

| Order | Proposed budget | Scope | Required result |
| --- | --- | --- | --- |
| 1 | 16–40 hours | String32 allocation, ownership and exception paths | A documented ABI plus an original/replacement create–append–free comparison, or a specific unresolved dependency |
| 2 | 16–40 hours after step 1 | One resource-name lookup path | A complete dependency slice with reproducible differential results |
| 3 | 8–24 hours | Second gameplay state and save/load acceptance | Controlled input-driven progress and a repeatable state round trip |
| 4 | Re-estimate from steps 1–3 | Resource, scene and object lifecycle | Measured throughput and an integration plan for a larger subsystem |
| 5 | Unscheduled | Full source engine and campaign | Engine execution without original game machine code, followed by campaign and save regression tests |
| 6 | Unscheduled | Port | A chosen platform, working compatibility layer, and platform-specific acceptance tests |

The first useful scheduling checkpoint is after a complete allocation/resource lifecycle has been rebuilt. Failed experiments should narrow the next task; they should not be hidden by increasing a progress bar.

## Completion criteria

For a **source-only engine**, the game must execute reconstructed or explicitly identified replacement/library source without calling the captured original executable. It may still consume separately supplied game assets; that dependency must be documented.

For a **tested game reconstruction**, scene progression, dialogue branches, input, save/load, audio, animation and rendering need representative regression coverage. Completing one opening-scene save cycle does not satisfy this criterion.

For a **port**, compilation alone is insufficient. The target platform must run the intended gameplay and pass its acceptance tests.

Additional CPU workers can accelerate independent analysis, builds and tests. GPU use has no established role in the current native-code reconstruction pipeline. Neither substitutes for recovering the missing contracts and behavior.
