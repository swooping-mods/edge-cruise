# Edge Cruise Upgrade: behavior and implementation

Describes the retained payload **0.2.0**. The behavior prose describes the retained payload; current candidate source and metadata are identified below. Gameplay qualification is a separate requirement.


<!-- source-only-sync -->
## Current source candidate

Source version **0.2.0** is saved from workshop revision `ca32bbf79ce0840d5166a232bdb3a9fdcd761254`. The retained installable payload remains **0.2.0** with its original bytes. This source update does not rebuild or requalify that payload. The current source and metadata define the candidate; gameplay, rendered behavior and co-op remain unqualified for rebuilt bytes.

The authored behavior and lifecycle descriptions below are retained from the published payload. They are not new qualification evidence for the source candidate. The integration table below follows the current candidate metadata.
<!-- /source-only-sync -->

## Behavior

Buy a $350 per-car Edge Cruise upgrade after boarding a cruise-equipped car. With cleaning and vanilla cruise already enabled, J engages edge steering. Every 0.1 simulated seconds the controller takes at most 32 dust-height samples across two forward rows, finds a unique cleared/dust transition and steers to overlap the cleaning edge. Vehicle Control Framework arbitrates its per-car assist lease against GPS and manual wheel input.

## State and lifecycle

A checked sidecar records up to 16 native storage IDs; only module ownership persists. Exact debit is confirmed before a grant; failed persistence requests a checked refund and uncertain transactions block further purchases. Manual steering/braking, J, focus/UI loss, leaving the car, authority change, stale callbacks or invalid edge geometry releases assistance and native cruise. Re-engagement is explicit.

## Integration and configuration

Runtime dependencies: [shop-framework](https://github.com/swooping-mods/shop-framework), [hints-framework](https://github.com/swooping-mods/hints-framework), [vehicle-control-framework](https://github.com/swooping-mods/vehicle-control-framework).

The [module metadata](../source/mods/edge-cruise/mod.json) owns the version and declared integration contract. Native modules start at `MoonLab_ModStart`. Game/Unity integrations target the pinned Steam Windows x64 IL2CPP build.

| Service | ABI | Table size |
|---|---:|---|
| Requires `shop-framework.shop` | 1 | at least 56 bytes |
| Requires `hints-framework.hints` | 1 | at least 24 bytes |
| Requires `vehicle-control-framework.controls` | 1 | at least 72 bytes |

## Implementation sources

- [mod.c](../source/mods/edge-cruise/mod.c)
- [edge_core.h](../source/mods/edge-cruise/edge_core.h)
- [ownership.h](../source/mods/edge-cruise/ownership.h)
- [store.h](../source/mods/edge-cruise/store.h)

## Verification boundary

Designed for straight strips and gentle bends: no obstacle planner, U-turns or automatic resumption. Ambiguous edges, tight turns and invalid speed/step inputs refuse guidance. Wallet and sidecar are separate stores, without a cross-store crash transaction.

`python3 build.py --check` verifies the exported source hashes. It does not execute the mod or qualify the behaviors above. Installation and independent build prerequisites are in the [README](../README.md).
