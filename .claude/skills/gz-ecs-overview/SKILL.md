---
name: gz-ecs-overview
description: Concise reference for the gz-sim Entity-Component-System architecture — how Entities, Components, Systems, the ECM, the Server, and SimulationRunner fit together. Trigger when the user asks how gz-sim is organized, where to add code, or how the simulation loop runs.
---

# gz-sim ECS architecture in 5 minutes

```
                ┌──────────────────────────────────────────┐
                │                 Server                   │
                │  (one process, owns N runners)           │
                └──────┬───────────────────────────────────┘
                       │
            ┌──────────▼──────────────────────────┐
            │         SimulationRunner            │
            │  - drives the iteration loop        │
            │  - owns the EventManager            │
            │  - owns the EntityComponentManager  │
            │  - owns the System list             │
            └──────────┬──────────────────────────┘
                       │ every step
   ┌───────────────────┼───────────────────────────────────────┐
   ▼                   ▼                   ▼                   ▼
Configure*       PreUpdate*           Update*             PostUpdate*
(once on load)   (mutates ECM)        (mutates ECM)       (read-only)
                                      └── Reset hooks on /reset
```

## The core types

| Type | Header | Role |
|------|--------|------|
| `Entity` | `Entity.hh` | Just a `uint64_t` ID. Means nothing on its own. |
| `components::Component<T, id>` | `components/Component.hh` | Strongly-typed data attached to an Entity. |
| `EntityComponentManager` | `EntityComponentManager.hh` | The world's database. Add/remove/query components. |
| `EventManager` | `EventManager.hh` | Pub/sub for cross-system events. |
| `System` (`ISystemConfigure`, `ISystemPreUpdate`, `ISystemUpdate`, `ISystemPostUpdate`, `ISystemReset`) | `System.hh` | Behaviour. Plugins implement one or more of these. |
| `SimulationRunner` | (internal) | Owns and drives the above. |
| `Server` | `Server.hh` | Public façade — `Run()`, `SetUpdatePeriod()`, etc. |

## Adding behaviour: pick a system phase

* **`Configure()`** — once on plugin load. Read SDF, cache handles, register
  topics. Do *not* mutate the world here unless you're spawning fixtures.
* **`PreUpdate()`** — every iteration, *before* physics. Apply commands
  (forces, joint targets), spawn entities.
* **`Update()`** — every iteration, alongside physics. Most user code does
  *not* belong here.
* **`PostUpdate()`** — every iteration, *after* physics. **Read-only ECM**
  access — emit telemetry, publish state, write logs. The ECM is `const`
  here for a reason.
* **`Reset()`** — when the world resets (e.g. via `/world/<name>/control`).
  Restore any internal state cached in the system.

## The "Cmd / state / Reset" component triplet

A common pattern in gz-sim:

* `<X>` — current value (e.g. `JointPosition`).
* `<X>Cmd` — request from a user system, consumed and cleared in
  `PreUpdate` by the physics-coupled system.
* `<X>Reset` — value to apply at the next world reset.

Look at `components/JointPositionReset.hh` / `JointVelocityCmd.hh` for the
canonical examples.

## Querying the ECM efficiently

Prefer `Each<>` views — they're cached and re-used across iterations:

```cpp
_ecm.Each<components::Joint, components::JointVelocity>(
  [&](const Entity &_ent,
      const components::Joint *,
      const components::JointVelocity *_vel) -> bool
  {
    // do work with *_vel
    return true; // keep iterating
  });
```

`EachNew`, `EachRemoved`, `EachChanged` exist for change-detection passes.

## Where things live

* **Component definitions** — `include/gz/sim/components/<Name>.hh`
  (header-only). Register serialization in `src/ComponentFactory.cc` when
  you want the component to survive log replay / save.
* **System plugins** — `src/systems/<snake_case>/<CamelCase>.{hh,cc}` plus
  an entry in `src/systems/CMakeLists.txt`. Use the `new-system` skill to
  scaffold.
* **High-level wrappers** — `Model`, `Link`, `Joint`, `Light`, `Actor`,
  `Sensor` (`include/gz/sim/`). These wrap an `Entity` plus ECM accessors
  and are usually how user code touches a model.

## Threading model

* `PreUpdate` / `Update` / `PostUpdate` of *different* systems run
  sequentially inside one runner iteration — not in parallel.
* `Server::Run(true, n)` blocks; `Run(false, n)` runs the loop on a worker
  thread. Multi-runner setups exist (different worlds), each on its own
  thread.
* Don't add long-running work in `Update()`. Spawn a thread in `Configure`,
  communicate via lock-free queues, drain in `PreUpdate`.

## Read the source, not just this

If you only have time for two files, read:

1. `include/gz/sim/EntityComponentManager.hh` — every comment matters.
2. `include/gz/sim/System.hh` — the interfaces are tiny and tell you
   exactly what each phase guarantees.
