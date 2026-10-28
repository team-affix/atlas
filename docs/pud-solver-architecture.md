# PUD solver architecture (decisions)

**Status:** Design direction agreed 2026-10-27. Not fully implemented yet (naming and first solver manifests are follow-ups).

## Context

Atlas CHC solvers (Horizon, Ridge, DBUCT, etc.) share a loop (`run_sim`) but **policy and resolution are coupled through per-solver manifests**: goal weights, MCTS, custom resolvers, and similar pieces are wired by dependency injection. There is no small, stable solver runtime API that hides what heuristics need.

PUD looked cleaner at first: `pud_runtime` exposes `register_module`, `unfold`, and limited introspection. The open question was how **automated drivers and PUD solvers** should choose unfolds without either leaking the whole engine through a growing public API, or treating one shared runtime as the extension point for every policy.

## Decisions

### 1. Persistence boundary: `pud_module` only

- **On disk:** binary `pud_module` blobs (module id, dependencies, axioms, stored inferences, refutation flags). **No solver-specific fields.**
- **Not on disk:** solver continuation (MCTS trees, visit counts, per-leaf statistics, RNG state, custom indexes).
- **Reload:** deserialize modules, register into a **new session**, rebuild the live frontier and queries via activation (`register_module` / `module_activator`). Live state is not read from the file.
- **Implication:** Any solver can run on the same module bytes at different times; each run recomputes ephemeral metrics from the current structural and live state.

Structural progress (what is unfolded) lives in the module. **Active frontier, MHQ, and query holders** are session live plane; see [pud-architecture.md](pud-architecture.md).

### 2. Do not grow `pud_runtime` for solvers

Adding accessors to a shared runtime for each new heuristic (witness counts, query heads, candidate topology, etc.) **is coupling**: the general surface changes whenever a solver needs something new. That repeats the mistake of a single observer API that must cover all future policies.

**Solvers should not depend on a fat shared PUD runtime interface.**

### 3. One composition root per solver (and per manual driver)

- Each **automated PUD solver** (and each distinct **manual driver**, if any) gets its own **`pud_<name>_manifest`** as the **entry point**, same pattern as `horizon_manifest` vs `ridge_manifest`.
- Manifests may **repeat wiring** of the shared kernel (unfolder, resolver, MHQ stack, `pud_modules`, etc.). **Duplication at the entry point is acceptable**; minimizing DRY across manifests is not a goal.
- Heuristic code may read **manifest internals directly** (query holders, MHQ, active states, modules registry). That is in-process coupling localized to that manifest, not serialized into modules or a global runtime API.

### 4. Manual stack vs solver stacks (planned naming)

Today's `pud_runtime` and `pud_manifest` are primarily a **harness** for integration tests and **manual** `(leaf, goal)` unfolding:

| Today (planned rename) | Role |
|------------------------|------|
| `pud_runtime` → **`pud_manual_runtime`** | Thin facade: register, unfold, test helpers (e.g. `get_rule_information`) |
| `pud_manifest` → **`pud_manual_manifest`** (optional rename) | Default kernel wiring used by manual runtime and tests |

**Automated solvers** use **disjoint** `pud_<solver>_runtime` and `pud_<solver>_manifest` types. They invoke the same kernel operations (e.g. `unfolder_.unfold`) inside their manifest; they do **not** go through `pud_manual_runtime`.

Integration tests that keep a `live` set and call `unfold` are the model for manual control; solvers internalize their own loop and session state.

### 5. Effect primitive, not a complete observation API

- **Stable semantic operation:** one step of world change—today **`unfold(leaf, body_goal_id)`**, which expands **all** resolver candidates for that goal (see `pud_unfolder`).
- **Typical driver choice:** pick **which leaf** and **which body goal**; candidate-level branching stays inside the kernel unless a specific solver manifest changes resolver or unfolder wiring.
- **No goal** to define a complete, solver-neutral read API on top of the engine.

### 6. Module access pattern (unchanged, intentional)

- Callers **own** `pud_module` (or test doubles).
- **`pud_modules<IModule>`** registers **`IModule&`**, stores **`IModule*`**, exposes tree operations via **`pud_global_node_id`**.
- Downstream components use **capability-style DI**; production usually passes the **same `modules_` registry** in many template slots.
- **Future reactive behavior** (e.g. intercept writes): provide an alternate **`IModule`** (or wrapper) that delegates to real storage and hooks `store_inference` / `mark_refuted`, without changing the unfolder's public shape.

Serde and CLI use **concrete `pud_module`**; the live engine uses the registry plus side tables (`pud_node_goals`, `pud_active_states`, etc.).

## What we explicitly rejected

- **Incremental observer methods on a shared runtime** as the way solvers get data.
- **One mega-manifest** to avoid duplicate wiring at the cost of opaque policy extension.
- **Solver-specific adjacency on disk** next to modules.

## Relation to CHC solvers

| CHC | PUD (direction) |
|-----|------------------|
| `db` + resolution infra | `pud_manifest` kernel + `pud_modules` |
| Per-solver manifest + `run_sim` | Per-solver manifest + driver loop (`unfold` steps) |
| Session state in memory | Session state in memory; **modules** persisted |
| Weak external boundary for heuristics | Same for heuristics **inside manifests**; **stronger** persistence boundary (`pud_module`) |

## Open follow-ups

- Rename manual types (`pud_manual_runtime`, optional `pud_manual_manifest`) and update test includes.
- First real solver manifest: session setup (which queries to open), frontier tracking, stats maps, stop condition; optional CLI runtime with `next()` mirroring `ridge_runtime`.
- Document operational "resume" as structural reload plus the same query-setup policy, not deserializing MHQ or solver state.

## References

- [pud-architecture.md](pud-architecture.md) — stored vs live planes, manifest composition.
- `core/hpp/infrastructure/pud_runtime.hpp` — current manual facade (to rename).
- `core/hpp/infrastructure/pud_manifest.hpp` — kernel composition root.
- `core/hpp/infrastructure/pud_modules.hpp` — `IModule` registry pattern.
