# Gameplay Interaction Framework for Unreal Engine 5

[![Unreal Engine](https://img.shields.io/badge/Unreal%20Engine-5.8%2B-blue?logo=unrealengine&logoColor=white)](https://www.unrealengine.com/)
[![Tests](https://img.shields.io/badge/Automated%20Tests-40%20Passing%20(100%25)-brightgreen)](USER_GUIDE.md)
[![Architecture](https://img.shields.io/badge/Architecture-Authoritative%20C%2B%2B%20%7C%20RepNotify-blue)](#key-highlights)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Author](https://img.shields.io/badge/Author-Shankharaj%20Datta-blue?logo=linkedin)](https://www.linkedin.com/in/shankharaj-datta-45921a28a/)

A high-performance, modular, and authoritative C++ interaction framework engineered for Unreal Engine 5 (UE 5.8+).

Designed from first principles to provide clean architectural separation, zero art asset bloat, strict multiplayer authority, and resilient cross-level state tracking.

---

## Key Highlights

- **Pure C++ Architecture & Zero Asset Bloat**: Completely decoupled from specific character classes, 3D meshes, textures, or audio cues. Integrate on any `AActor`, `APawn`, or `ACharacter` via modular components and interfaces.
- **Authoritative Multiplayer First**: Built on atomic contention locks and **RepNotify-driven replication** (`FInteractionReplicatedState` with rolling `ChangeCounter`, zero Multicast RPCs) ensuring late-joiner consistency and network bandwidth efficiency.
- **Anti-Cheat & Lag-Tolerant Hold Verification**: Server validates hold timing using instigator round-trip latency (`NetworkHoldTimeTolerance`) while preventing client speedhack exploits.
- **Crash & Disconnect Resiliency**: Leased lock TTL auto-expiration prevents lock starvation. Dynamic binding to instigator destruction (`OnEndPlay`) immediately releases acquired targets on disconnects or crashes.
- **3-Phase Async Commit Transaction Safety**: Supports asynchronous backend queries, inventory transactions, and save commits via `BeginAsyncInteraction`, `CommitAsyncInteraction`, and `RollbackAsyncInteraction` protected by unique `FGuid` session identifiers.
- **Blueprint Async Action Node**: Intuitive latent workflow via `PerformInteractionAsync` node exposing execution pins: `OnStarted`, `OnProgress`, `OnCompleted`, and `OnFailed`.
- **Metroidvania Cross-Level Persistence**: Game Instance Subsystem (`UInteractionPersistenceSubsystem`) tracks state changes across level streaming, World Partition cells, and player map travel across both `WorldShared` and `InstigatorScoped` domains.
- **Multi-Switch Coordination**: Built-in verification logic for complex cooperative puzzles (e.g. 4 players triggering 4 switches across distinct levels within a configurable time window).
- **Extensible Tracing & Candidate Scoring**: Pluggable trace strategies (`IInteractionTraceStrategy`) with weighted multi-factor scoring (angle dot, distance, and designer priority).
- **Tag-Driven Animation Synchronization**: Minimalist, decoupled `UAnimNotify_InteractionEvent` and `UAnimNotifyState_InteractionEvent` classes dispatching GameplayTags (`Interaction.Event.*`, `Interaction.CancelReason.*`) to seamlessly sync montages, continuous windows, and dodge/hit interruptions with zero hardcoded animation assets.
- **Verified Test Coverage**: 40 comprehensive automated unit, authority, animation, and stress tests passing headless with 100% success rate.

---

## Architectural Overview

```text
[ Client / Player Controller / Character ]
  │
  ├── UInteractionDetectorComponent
  │     ├── IInteractionTraceStrategy (CameraLine, ForwardCone, Custom)
  │     ├── Directional Scoring Pipeline (Angle + Proximity + Priority)
  │     └── Input State Machine (Instant, Hold, RapidTap, Toggle)
  │
  ├── UInteractionLocalPlayerSubsystem ──► UInteractionPromptWidgetBase (UI HUD)
  └── UAsyncAction_PerformInteraction (Latent Blueprint Node)

[ Server Authority & World State ]
  │
  ├── UInteractableComponent (Placed on any Actor / Prop / NPC)
  │     ├── Smart Interaction Points (Sockets / Local Offsets)
  │     ├── Server Validation (Range, LoS Occlusion, Tag Queries)
  │     ├── Atomic Contention Lock & TTL Lease Timer
  │     └── Instigator OnEndPlay Crash Hook
  │
  ├── UInteractionWorldSubsystem (Spatial Registry & Fast Broad-Phase)
  └── UInteractionPersistenceSubsystem (Cross-Level Global State Ledger)
```

---

## Plugin Directory Structure

```text
GameplayInteractionFramework/
├── Config/
│   └── FilterPlugin.ini
├── Resources/
│   ├── Icon128.png                     # Plugin marketplace & browser icon
│   └── Icons/                          # High-DPI Slate class icons & thumbnails
│       ├── InteractableComponent_16.png
│       ├── InteractableComponent_64.png
│       ├── InteractableComponent.svg
│       ├── InteractionDetectorComponent_16.png
│       ├── InteractionDetectorComponent_64.png
│       └── InteractionDetectorComponent.svg
├── Source/
│   ├── GameplayInteractionFramework/   # Core runtime module
│   │   ├── Public/
│   │   │   ├── Async/                  # Latent BP Async Action
│   │   │   ├── Components/             # Actor components
│   │   │   ├── FunctionLibraries/      # Utility libraries
│   │   │   ├── Interfaces/             # Target, instigator & trace interfaces
│   │   │   ├── Settings/               # UDeveloperSettings
│   │   │   ├── Subsystems/             # World, LocalPlayer, and GameInstance subsystems
│   │   │   ├── Types/                  # Native tags, structs, and enums
│   │   │   └── UI/                     # Prompt widget base
│   │   └── Private/                    # Implementation files
│   └── GameplayInteractionFrameworkTests/ # Automated regression suite
│       └── Private/Tests/              # 36 automated unit & stress tests
├── LICENSE                             # License terms
├── README.md                           # Overview & architectural reference
├── USER_GUIDE.md                       # Complete step-by-step user manual
└── GameplayInteractionFramework.uplugin # Plugin descriptor
```

---

## Core Components Reference

### 1. `UInteractableComponent`
Actor component added to any world object that can be interacted with (doors, chests, levers, terminals, NPCs, loot).
- **Smart Points**: Define multiple interaction sockets or local coordinate offsets (e.g. front handle vs rear handle).
- **Tag Validation**: Evaluates required and blocked tags on both instigator and target.
- **State Lifecycle**: Manages `Available`, `Active`, `Cooldown`, `Completed`, `Disabled`, and `PendingCommit` states.
- **TTL Lease Protection**: Automatically recovers from abandoned interactions if an instigator stalls or desyncs.

### 2. `UInteractionDetectorComponent`
Actor component attached to player characters, controllers, or AI pawns.
- **Perception Sweep**: Executes raycasts or shape sweeps using pluggable trace strategies.
- **Directional Scoring**: Evaluates candidate targets using angle dot product, distance, and priority weight parameters.
- **Input Modes**: Handles Instant triggers, Hold execution with duration timing, RapidTap progress accumulation with decay, and Toggle state flips.

### 3. `UInteractionPersistenceSubsystem`
Game Instance subsystem that outlasts level unloads, world transitions, and player travel.
- **WorldShared Ledger**: Global mission milestones, unlocked doors, drained water gates.
- **InstigatorScoped Ledger**: Personal loot chests, player-specific mission objectives.
- **Stream-In Hydration**: Automatically restores state on `BeginPlay` when actors stream in via World Partition without firing redundant network events.

---

## Automated Verification

The framework includes a comprehensive test suite in the `GameplayInteractionFrameworkTests` module.

Run the test suite headless using Unreal Engine:
```bash
UnrealEditor-Cmd.exe "<ProjectPath>/<ProjectName>.uproject" -ExecCmds="Automation RunTests GameplayInteractionFramework; Quit" -stdout -FullStdOutLogOutput -unattended -nopause -NullRHI
```

**Test Coverage Summary (40 Passing Tests)**:
- Native tag registration and hierarchical tag tree verification.
- Smart point selection, coordinate transformation, and candidate scoring algorithms.
- Full input state machines: Instant, Hold, RapidTap accumulation/decay, Toggle.
- Network authority validation: distance gating, line-of-sight occlusion, tag gating.
- Atomic contention locks and concurrent player rejection.
- Anti-speedhack hold timing verification with latency compensation.
- Leased lock TTL auto-expiration and disconnect/crash cleanup via `OnEndPlay`.
- 3-Phase async commit transaction lifecycle (`PendingCommit` -> `Commit` / `Rollback`).
- Cross-level persistence ledger and multi-switch cooperative validation.
- Spatial broad-phase candidate queries and query throttle frequency benchmarks.
- Tag-driven animation synchronization: instant notify event commits, custom gameplay event payloads, notify state interaction windows, and montage interruption cancellations.

---

## Getting Started

Refer to [USER_GUIDE.md](USER_GUIDE.md) for full configuration, Blueprint examples, and integration patterns.

---

## License & Attribution

Distributed under the **MIT License**. See [LICENSE](LICENSE) for details.

Created by **[Shankharaj Datta](https://www.linkedin.com/in/shankharaj-datta-45921a28a/)**.
