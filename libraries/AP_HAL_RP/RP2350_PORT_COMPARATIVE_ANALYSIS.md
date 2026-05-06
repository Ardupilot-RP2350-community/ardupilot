# RP2350 ArduPilot Port Comparative Analysis (C1 vs C2 vs C3)

Date: 2026-04-10  
Author: AI-assisted analysis (compiled from public branch metadata, file trees, and in-repo technical documentation)

## 1) Scope

This report compares three RP2350-focused ArduPilot forks/branches:

1. **C1**: `Ardupilot-RP2350-community/ardupilot` @ `private/rp2350`  
   https://github.com/Ardupilot-RP2350-community/ardupilot/tree/private/rp2350
2. **C2**: `zsigmondszilveszter/ardupilot-rpi-pico` @ `rp2xxx-chibios`  
   https://github.com/zsigmondszilveszter/ardupilot-rpi-pico/tree/rp2xxx-chibios
3. **C3**: `davidbuzz/ardupilot` @ `buzz-rp2350-chibios-v4`  
   https://github.com/davidbuzz/ardupilot/tree/buzz-rp2350-chibios-v4

## 2) Methodology and Evidence Basis

Because direct git-network access was unavailable in this environment, this comparison is based on:

- Branch compare metadata visible on GitHub compare pages (contributors / commit count / files changed).
- Repository tree inspection (HAL directories and naming).
- C3's in-branch technical overview document:
  - `TECHNICAL_OVERVIEW_AND_COMPLETENESS-RP2350-implementation-comparison.md`

> Important: several low-level feature claims for C1/C2/C3 are self-reported in C3's technical overview and were treated as **reported evidence**, not independently hardware-verified here.

## 3) High-level Comparative Snapshot

| Dimension | C1 (`private/rp2350`) | C2 (`rp2xxx-chibios`) | C3 (`buzz-rp2350-chibios-v4`) |
|---|---|---|---|
| Compare-to-master size (GitHub compare) | 40 commits, 85 files, 4 contributors | 798 commits, 987 files, 31 contributors | 391 commits, 225 files, 3 contributors |
| HAL architecture | **Dedicated HAL**: `libraries/AP_HAL_RP` | **Dedicated HAL**: `libraries/AP_HAL_rp2xxxChibiOS` | **Integrated into ChibiOS HAL** (`AP_HAL_ChibiOS`) |
| RTOS/BSP approach | FreeRTOS + Pico SDK (as described in C3 doc) | Custom rp2xxx ChibiOS fork | Upstream ChibiOS-centric RP2350 enablement |
| Platform strategy | New RP-only HAL stack | New rp2xxx-specific HAL stack | Keep standard ArduPilot HAL path and extend RP2350 support there |
| Integration risk to upstream ArduPilot | Medium/High (parallel HAL) | Medium/High (parallel HAL + forked ChibiOS path) | Lower/Medium (fewer architectural departures) |

## 4) Architectural Differences (Most Important)

### C1: `AP_HAL_RP` (new HAL family)
- A standalone HAL subtree exists at `libraries/AP_HAL_RP`, with broad driver coverage (I2C/SPI/UART/RC IO/Storage/etc. listed in tree).
- This is architecturally similar to creating a new platform family beside existing HALs.
- Strength: tight RP2350 focus and fast RP-specific iteration.
- Tradeoff: larger long-term maintenance burden and potential drift from mainstream ChibiOS HAL patterns.

### C2: `AP_HAL_rp2xxxChibiOS` (new HAL, ChibiOS-based)
- A separate HAL exists at `libraries/AP_HAL_rp2xxxChibiOS`.
- It appears to rely on custom rp2xxx ChibiOS integration rather than upstream `AP_HAL_ChibiOS` flow.
- Strength: explicit rp2xxx tuning and board-level freedom.
- Tradeoff: dual-maintenance risk (custom HAL + ChibiOS specialization) and higher merge friction.

### C3: RP2350 work inside `AP_HAL_ChibiOS`
- No separate `AP_HAL_RP` or `AP_HAL_rp2xxxChibiOS` directory is present in the branch tree.
- RP2350 appears handled by extending the existing ChibiOS HAL board/port infrastructure.
- Strength: strongest alignment with upstream ArduPilot structure and existing ChibiOS board workflow.
- Tradeoff: possibly slower initial bring-up than greenfield HAL work, but better long-term convergence.

## 5) Reported Maturity Signals

Using available compare metadata + C3's published technical report:

- **C2** has the largest historical divergence (798 commits / 987 files changed), suggesting a broad and long-lived experimental branch.
- **C3** shows concentrated RP2350 development with moderate divergence (391 commits / 225 files changed) and extensive written bring-up notes.
- **C1** appears most compact by compare metrics (40 commits / 85 files changed) but includes a full dedicated HAL tree (`AP_HAL_RP`), indicating focused but architectural-heavy changes.

## 6) Completeness vs Upstreamability (Assessment)

### If priority is fastest RP-specific experimentation
- **C1** or **C2** style (separate HAL) can move quickly on RP-specific features.

### If priority is eventual upstream mergeability and maintenance
- **C3** strategy (extend `AP_HAL_ChibiOS`) is generally preferable because it minimizes parallel HAL ecosystems.

### Practical ranking by likely upstream merge friction (lowest to highest)
1. **C3** (integrated ChibiOS path)
2. **C1** (new HAL)
3. **C2** (new HAL + large historical divergence)

> This ranking is about integration/maintenance cost, not absolute code quality.

## 7) Recommendations

1. Use **C3 architecture as baseline** for upstream-facing work.
2. Mine **C1/C2** for proven RP2350 driver ideas and board support details, but avoid long-term HAL fragmentation.
3. Standardize evidence quality across all efforts using the same checklist:
   - build proof (`./waf configure`, target build)
   - bench proof (sensor probe logs, UART loopback, RC timing captures)
   - flight proof (short controlled hover/drive logs)
4. Publish a unified RP2350 feature matrix (driver present / bench-verified / flight-verified) tied to exact commit hashes.

## 8) Source Links

- C1 branch root: https://github.com/Ardupilot-RP2350-community/ardupilot/tree/private/rp2350
- C2 branch root: https://github.com/zsigmondszilveszter/ardupilot-rpi-pico/tree/rp2xxx-chibios
- C3 branch root: https://github.com/davidbuzz/ardupilot/tree/buzz-rp2350-chibios-v4
- C1 compare: https://github.com/Ardupilot-RP2350-community/ardupilot/compare/master...private/rp2350
- C2 compare: https://github.com/zsigmondszilveszter/ardupilot-rpi-pico/compare/master...rp2xxx-chibios
- C3 compare: https://github.com/davidbuzz/ardupilot/compare/master...buzz-rp2350-chibios-v4
- C3 technical overview doc: https://github.com/davidbuzz/ardupilot/blob/buzz-rp2350-chibios-v4/TECHNICAL_OVERVIEW_AND_COMPLETENESS-RP2350-implementation-comparison.md
