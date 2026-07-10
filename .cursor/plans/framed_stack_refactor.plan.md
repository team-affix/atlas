---
name: framed journal refactor
overview: "Replace the virtual i_backtrackable trail journal on feature/add-dbuct with per-component frame stacks (like CDCL already does). No shared framed_stack<T> type — each reversible struct owns std::stack<frame> where frame holds a list/vector/set of value-typed actions. A dbuct_frame_hub coordinates push_frame/pop_frame across components. Root out i_backtrackable, backtrackable_*, backtrackable_mutation, tracked, and trail entirely."
todos:
  - id: phase0-baseline
    content: "Branch off feature/add-dbuct. Record Pythagorean benchmark (sims/sec) and optional Callgrind baseline on atlas_profile."
    status: pending
  - id: phase1-action-vos
    content: "Add value_objects action headers + per-component *_action.hpp variant aliases. Port undo semantics from each backtrackable_* type. Unit-test undo_action() per action type."
    status: pending
  - id: phase2-frame-hub
    content: "Add dbuct_frame_hub (push_frame, pop_frame, squash_frame, depth). Wire in dbuct_ridge_manifest. Replace trail_savepoint with frame_savepoint targeting hub."
    status: pending
  - id: phase3-migrate-simple
    content: "Migrate simple dbuct_* components (frame_bump_allocator, decision_memory, resolution_memory, unit_goals, goal_exprs, candidate_frame_offsets, nearest_decision, chosen_goal_candidates, elimination_backlog, avoidance_unit_boundary)."
    status: pending
  - id: phase4-migrate-complex
    content: "Migrate dbuct_goal_candidate_rules, dbuct_series_reduced_tree, dbuct_srt_active_goals, dbuct_bind_map, dbuct_mhu_elimination_generator."
    status: pending
  - id: phase5-sim-and-delete
    content: "Update dbuct_sim to use frame_hub. Delete trail, tracked, i_backtrackable, all backtrackable_* files. Fix all tests."
    status: pending
  - id: phase6-non-dbuct
    content: "Migrate ridge/basic/horizon elimination_backlog + sequencer + set_up_sim/tear_down_sim off trail (or defer with explicit note)."
    status: pending
  - id: phase7-validate
    content: "core_debug green. Pythagorean benchmark >= master. Callgrind shows zero vtable backtrack/invoke."
    status: pending
isProject: false
---

# Framed Journal Refactor — AI Handoff Plan

**Branch:** `feature/add-dbuct`  
**Do not touch:** `master`, existing non-dbuct manifests' behavior (until phase 6)  
**Hard rule:** Delete `i_backtrackable`, all `backtrackable_*`, `backtrackable_mutation`, `tracked`, and `trail`. No virtual undo. No `make_unique` per mutation.

---

## 1. Motivation

### Problem

`feature/add-dbuct` journals every state mutation through a shared `trail` that stores `std::unique_ptr<i_backtrackable>`. Each mutation pays:

1. `make_unique<backtrackable_*>` (heap allocation)
2. Virtual `invoke()` on forward path
3. Virtual `backtrack()` on `trail::pop()` — **the dominant cost**

Callgrind (20s, Pythagorean `dbuct-ridge`, `atlas_profile`):

| Hotspot | Feature | Master |
|---------|---------|--------|
| `::backtrack()` vtable | **839M Ir (~20%)** | 0 |
| `tracked::mutate` | 446M (~10.6%) | — |
| `trail::pop` | 299M (~7.1%) | — |
| `::invoke()` vtable | 196M (~4.6%) | — |
| Trail mechanism total | **~47% of instructions** | — |
| Heap alloc | 257M *fewer* than master | — |

The feature branch is **8–22% slower** than master on the benchmark despite asymptotically better CDCL. The trail *mechanism* loses; CDCL (which never uses trail) is already fast.

### What already works

`dbuct_cdcl_elimination_generator` is the template to copy:

```cpp
struct frame {
    std::list<avoidance_action> actions;
    std::list<raised_unit_avoidance> raised_unit_avoidance_lump;
};
std::stack<frame> frame_stack_;

void push_frame() { frame_stack_.push(frame{}); }

coroutine<const resolution_lineage*, void> pop_frame() {
    auto current = std::move(frame_stack_.top());
    frame_stack_.pop();
    for (auto it = current.actions.rbegin(); it != current.actions.rend(); ++it)
        undo_action(*it);
    // ... exotic: bubble unit avoidances into parent frame ...
}
```

- Plain value-typed `avoidance_action` variant (no vtable)
- Actions stored in a per-frame container (`std::list`)
- `undo_action()` dispatches via `std::get_if`
- Frame struct can hold **extra per-frame state** beyond the action log (`raised_unit_avoidance_lump`)
- `pop_frame()` can do **exotic work** (yield eliminations, mutate parent frame) that no generic stack abstraction would anticipate

**Every other reversible `dbuct_*` component should look like this.**

---

## 2. Architectural Decision

### Do NOT create a shared `framed_stack<T>` (or `trail<T>`) type

Each structure that needs reversibility owns its own frame stack inline. The stack is just:

```cpp
std::stack<ComponentFrame> frame_stack_;
```

where `ComponentFrame` is a struct the component defines — typically containing a `std::list<Action>` or `std::vector<Action>`, but open to whatever fits:

| Component | Suggested frame contents | Why not generic |
|-----------|-------------------------|-----------------|
| `dbuct_goal_exprs` | `std::list<goal_expr_action>` | Simple |
| `dbuct_cdcl` | `actions` + `raised_unit_avoidance_lump` | Pop bubbles state into parent |
| `dbuct_mhu` | `actions` + maybe arena-destruction markers | Transaction squash merges frames |
| `dbuct_series_reduced_tree` | `std::vector<srt_action>` | Could coalesce canceling ops before pop |
| Future | Actions appended to *parent* frame mid-episode | CDCL already does this with `raised_unit_avoidance_lump` |

**Benefits of no shared type:**

- Implementations choose list vs vector vs deque per workload
- Components can append actions to **prior** frames when needed
- Components can **reduce/collapse** canceling action pairs at pop time (e.g. insert+erase same key → no-op)
- `pop_frame()` return type varies: `void`, `coroutine<...>`, etc.
- No false abstraction forcing CDCL-shaped semantics on everyone

### What each component implements

Every reversible `dbuct_*` struct gains:

```cpp
void push_frame();
void pop_frame();              // or coroutine<...> if it yields side effects
void squash_frame();           // optional — for savepoint commit; not all components need it
```

Plus private:

```cpp
void undo_action(const ComponentAction&);   // std::get_if dispatch, like CDCL
void log_action(ComponentAction);           // append to frame_stack_.top()
```

**Remove** the `ILogTrailAction` template parameter from every `dbuct_*` header. Components are self-contained.

### Action types — coding standard

Mirror `avoidance_action` / `avoidance_watcher_update` / `avoidance_unwatch`:

```
core/hpp/value_objects/
  <semantic_action>.hpp       — one plain struct per action alternative
  <component>_action.hpp      — using ComponentAction = std::variant<...>;
```

- One header per variant alternative (e.g. `goal_expr_insert.hpp`)
- One header per component variant alias (e.g. `goal_expr_action.hpp`)
- Structs should be comparable (`operator<=>` = default) where tests need equality
- Action structs carry **undo data** (keys, captured values, amounts) — not live pointers into state

**Shared action headers are OK** when undo semantics are identical (e.g. a reusable `trail_map_insert` struct used in multiple variant aliases). Per-component variant aliases still get their own `*_action.hpp`.

---

## 3. Frame Coordination — `dbuct_frame_hub`

### Problem

Today one shared `trail` object is both the undo journal for all state **and** the frame counter (`trail.depth()`). `dbuct_sim` pushes/pops it in lockstep with `cdcl.push_frame()` / `cdcl.pop_frame()`. `dbuct_avoidance_unit_boundary` reads `trail.depth()` for unit-boundary indexing.

With per-component stacks, something must still keep frames aligned.

### Solution

New struct `dbuct_frame_hub` owned by `dbuct_ridge_manifest`:

```cpp
struct dbuct_frame_hub {
    void push_frame();    // call push_frame() on every registered component
    void pop_frame();     // call pop_frame() on every registered component (define order)
    void squash_frame();  // call squash_frame() on every registered component (savepoint commit)
    size_t depth() const; // canonical frame depth (1-based at root, matching today's trail.depth())
};
```

- Holds references to every reversible `dbuct_*` member + CDCL is **separate** (already has own stack)
- `depth()` is a simple `size_t depth_` incremented/decremented in push/pop, or derived from one canonical component
- `dbuct_sim` replaces all `trail_.push/pop/depth` with `hub_.push_frame/pop_frame/depth`
- `dbuct_avoidance_unit_boundary` takes `IFrameDepth&` (implemented by hub) instead of `ILogTrailAction&`

### Pop order

Pop dependents before dependencies (reverse of init order). Suggested:

1. `dbuct_avoidance_unit_boundary`
2. `dbuct_nearest_decision`
3. `dbuct_mhu_elimination_generator`
4. `dbuct_bind_map` (common)
5. `dbuct_chosen_goal_candidates`
6. `dbuct_goal_candidate_rules`
7. `dbuct_goal_exprs`
8. `dbuct_srt_active_goals`
9. `dbuct_unit_goals`
10. `dbuct_decision_memory`
11. `dbuct_resolution_memory`
12. `dbuct_candidate_frame_offsets`
13. `dbuct_frame_bump_allocator`
14. `dbuct_elimination_backlog`

CDCL `pop_frame()` is still called separately by `dbuct_sim` (it yields eliminations). Hub pop handles only the journal-backed state components.

### `frame_savepoint` (replaces `trail_savepoint`)

```cpp
template<typename IFrameControl>
struct frame_savepoint {
    explicit frame_savepoint(IFrameControl&);  // push_frame()
    ~frame_savepoint();                        // pop_frame() if !committed
    void commit();                             // squash_frame()
};
```

`dbuct_frame_hub` implements `IFrameControl`. Used by MHU `try_add_head` transactional bind.

**Per-component `squash_frame()` semantics:** Default = merge top frame's action list into parent frame's list (equivalent to old `trail::squash_one`). Components may override with smarter collapse (drop canceling pairs when merging).

---

## 4. Exemplar Migration — `dbuct_goal_exprs`

### Before

```cpp
template<typename ILogTrailAction>
struct dbuct_goal_exprs {
    tracked<map_t, ILogTrailAction> exprs_;
    void set(gl, fe) { exprs_.mutate(make_unique<backtrackable_map_insert<...>>(...)); }
};
```

### After

```cpp
struct dbuct_goal_exprs {
    void push_frame() { frame_stack_.push({}); }
    void pop_frame() {
        auto frame = std::move(frame_stack_.top());
        frame_stack_.pop();
        for (auto it = frame.actions.rbegin(); it != frame.actions.rend(); ++it)
            undo_action(*it);
    }
    void squash_frame() {
        auto top = std::move(frame_stack_.top());
        frame_stack_.pop();
        auto& parent = frame_stack_.top().actions;
        parent.splice(parent.end(), std::move(top.actions));  // list merge
    }

    framed_expr get(const goal_lineage* gl) const { return exprs_.at(gl); }
    void set(const goal_lineage* gl, framed_expr fe);
    void unset(const goal_lineage* gl);

private:
    struct frame {
        std::list<goal_expr_action> actions;
    };

    void undo_action(const goal_expr_action&);
    void log(goal_expr_action);

    map_t exprs_;
    std::stack<frame> frame_stack_;
};
```

`set()`:

```cpp
void dbuct_goal_exprs::set(const goal_lineage* gl, framed_expr fe) {
    auto [_, inserted] = exprs_.insert({gl, std::move(fe)});
    DEBUG_ASSERT(inserted);
    log(goal_expr_action{goal_expr_insert{gl, fe}});
}
```

`undo_action()` for insert → `exprs_.erase(key)`. For erase → re-insert captured value.

**Optional exotic:** In `log()`, if top frame already has an insert for the same key, replace it instead of appending (collapse within frame). Only if profiling shows benefit — not required for v1.

---

## 5. Full Component Inventory

### 5.1 Reversible dbuct_* components

| Component | State owned | Action types | Notes |
|-----------|-------------|--------------|-------|
| `dbuct_goal_exprs` | `unordered_map<gl*, framed_expr>` | insert, erase | Simple |
| `dbuct_goal_candidate_rules` | `map<gl*, ra_rule_id_set>` | insert, erase, at_ra_insert, at_ra_erase | RA inner sets: re-lookup `at(key)` on undo |
| `dbuct_chosen_goal_candidates` | `map<gl*, rule_id>` | insert, assign | assign = swap undo |
| `dbuct_decision_memory` | `set<rl*>` | insert | |
| `dbuct_resolution_memory` | `set<rl*>` | insert | |
| `dbuct_unit_goals` | `vector<gl*>` | push_back, pop_back | |
| `dbuct_candidate_frame_offsets` | `map<rl*, uint32_t>` | insert, erase | |
| `dbuct_frame_bump_allocator` | `uint32_t` | add | |
| `dbuct_nearest_decision` | `map<rl*, rl*>` | insert | |
| `dbuct_elimination_backlog` | nested map/set | insert, at_insert | |
| `dbuct_avoidance_unit_boundary` | 4 scalars/pointers | assign × 4 | Reads `hub.depth()` on log |
| `dbuct_series_reduced_tree` | 4 containers | many map/set ops | Highest action variety |
| `dbuct_srt_active_goals` | tree + in_flight set | delegates to tree + set_insert/clear | |
| `dbuct_bind_map` | `unordered_map<uint32_t, framed_expr>` | insert, assign | Logs directly (no tracked). Per-head instances in MHU |
| `dbuct_mhu_elimination_generator` | arena deque + 3 maps | deque_emplace, map ops | Savepoint + exotic squash |

### 5.2 Already correct — do not change behavior

| Component | Notes |
|-----------|-------|
| `dbuct_cdcl_elimination_generator` | Reference impl. Optional: extract action logging helpers. **Do not regress.** |

### 5.3 Infrastructure to update

| File | Change |
|------|--------|
| `dbuct_sim.hpp` | `hub_` instead of `trail_`; `at_root()` → `hub_.depth() == 1` |
| `dbuct_ridge_manifest.hpp` | Replace `trail trail_` with `dbuct_frame_hub hub_`; remove `ILogTrailAction` template args |
| `trail_savepoint.hpp` | Rename → `frame_savepoint.hpp`, target hub |
| `set_up_sim.hpp` | Phase 6: push hub/frame instead of trail |
| `tear_down_sim.hpp` | Phase 6: unchanged semantics for non-dbuct |

### 5.4 Files to DELETE

```
core/hpp/interfaces/i_backtrackable.hpp
core/hpp/infrastructure/backtrackable_mutation.hpp
core/hpp/infrastructure/backtrackable_*.hpp          (17 files)
core/hpp/infrastructure/tracked.hpp
core/hpp/infrastructure/trail.hpp
core/cpp/infrastructure/trail.cpp
core/hpp/infrastructure/trail_savepoint.hpp          (replaced by frame_savepoint.hpp)
```

---

## 6. Semantic Preservation Checklist

These caused bugs in the original `backtrackable_*` design — preserve exactly:

| Operation | Undo rule |
|-----------|-----------|
| `map_insert` | `erase(key)` |
| `map_erase` | capture `mapped_type` on forward; `insert({key, value})` on undo |
| `map_assign` | swap-based (store other half) |
| `map_at_insert` | `at(key).erase(value)` |
| `map_at_erase` | `at(key).insert(value)` |
| `map_at_ra_insert/erase` | **Re-lookup** `map.at(outer_key)` on undo — inner container address may change if outer node erased/re-inserted same frame |
| `set_insert` | `erase(elem)` |
| `set_clear` | restore moved copy of entire set |
| `vector_push_back` | `pop_back()` |
| `vector_pop_back` | `push_back(captured)` |
| `deque_emplace_back` | `pop_back()` — MHU arena relies on deque stability |
| `add` | subtract amount |
| `increment` | decrement |
| `assign` | swap back |

Port existing unit tests from `core/test/unit/infrastructure/backtrackable_*.cpp` into per-action or per-component tests before deleting those files.

---

## 7. Special Cases

### 7.1 `dbuct_bind_map`

- Today logs directly to shared trail (bypasses `tracked`)
- After: owns `std::stack<frame>` with `bind_map_action` variant
- Manifest-level common bind_map has its own stack
- MHU `try_add_head` creates ephemeral bind_map — its actions must log into **MHU's frame** (or MHU forwards bind actions into its own stack). Simplest: ephemeral bind_map holds a reference to MHU's current frame list, or MHU wraps bind calls.

### 7.2 MHU `try_add_head` transaction

```cpp
frame_savepoint savepoint(hub_);
// arena emplace, bind mutations, heads map insert ...
savepoint.commit();  // hub.squash_frame() — merges all component frames
return false;        // ~savepoint → hub.pop_frame() on unify failure
```

All components participating in the transaction must have been push_frame'd (hub pushes all). Squash merges top into parent per component.

### 7.3 `dbuct_series_reduced_tree`

- 4 containers, 15+ mutation sites today
- Single `std::stack<srt_frame>` where `srt_frame::actions` is `std::vector<srt_action>` (many ops per reduce)
- **Good candidate for exotic collapse:** coalesce `{insert k, erase k}` in same frame during `pop_frame` before calling `undo_action`

### 7.4 Non-dbuct manifests (ridge/basic/horizon/ genius)

Still use `trail` + `tracked` + `tear_down_sim` today. Phase 6 options:

1. **Minimal:** Give each a simple `std::stack<std::list<ridge_action>>` in a local helper — still no shared type
2. **Defer:** Leave non-dbuct on old trail until dbuct path is green (acceptable if old files compile behind a compat shim temporarily — but end state is full deletion)

---

## 8. Implementation Phases

### Phase 0 — Baseline
- Branch: `cursor/framed-journal-refactor-997b` off `feature/add-dbuct`
- Run: `./build/atlas dbuct-ridge ./cli/examples/arithmetic/db.chc -g "mul(X,X,X2),..." --sim-progress-interval 100`
- Record sims/sec. Optional Callgrind on `atlas_profile`.

### Phase 1 — Action value objects
- Add `value_objects/*_action.hpp` and semantic action headers
- Implement `undo_action()` free functions or component methods
- Unit tests per action (port from `backtrackable_*.cpp` tests)
- **No consumers yet**

### Phase 2 — `dbuct_frame_hub` + `frame_savepoint`
- Add `dbuct_frame_hub.hpp`, `frame_savepoint.hpp`
- Wire into manifest (components still on old trail temporarily OR big-bang)

### Phase 3 — Simple components
Migrate in any order; each must compile + test before moving on:
- `dbuct_frame_bump_allocator`
- `dbuct_decision_memory`, `dbuct_resolution_memory`
- `dbuct_unit_goals`
- `dbuct_goal_exprs`
- `dbuct_candidate_frame_offsets`
- `dbuct_nearest_decision`
- `dbuct_chosen_goal_candidates`
- `dbuct_elimination_backlog`
- `dbuct_avoidance_unit_boundary`

Each migration step for one component:
1. Add frame struct + `std::stack<frame>`
2. Add `push_frame` / `pop_frame` / `squash_frame`
3. Replace `tracked::mutate(make_unique<backtrackable_*>)` with direct mutate + `log(action)`
4. Remove `ILogTrailAction` template param
5. Register in hub
6. Update unit tests

### Phase 4 — Complex components
- `dbuct_goal_candidate_rules`
- `dbuct_series_reduced_tree` → `dbuct_srt_active_goals`
- `dbuct_bind_map` + factory
- `dbuct_mhu_elimination_generator`

### Phase 5 — Sim + delete legacy
- `dbuct_sim.hpp` → hub
- Delete all files in §5.4
- Fix all compile errors
- `make core_debug && ./build/core_debug`

### Phase 6 — Non-dbuct paths
- `elimination_backlog`, `sequencer`, `set_up_sim`, manifests

### Phase 7 — Validate
- All tests green
- Benchmark ≥ master throughput
- Callgrind: `::backtrack()` / `::invoke()` vtable cost → **0**

---

## 9. Test Migration

| Delete | Replace with |
|--------|--------------|
| `backtrackable_*.cpp` (14 unit tests) | Per-action or per-component undo tests |
| `tracked.cpp` (unit + integration) | Component frame push/pop tests |
| `trail.cpp` | **Delete** — no shared trail type to test |
| `integration/backtrackable_increment.cpp` | `dbuct_frame_bump_allocator` or action test |
| Tests with `MOCK_METHOD(log, (unique_ptr<i_backtrackable>))` | Test via observable state after `pop_frame()`, or mock hub |

**Testing rule** (per `docs/testing.md`): prove undo by pushing a frame, mutating, popping, asserting state restored. For CDCL-style components that yield on pop, use coroutine collection pattern from `dbuct_cdcl_elimination_generator` tests.

---

## 10. Success Criteria

1. Zero references to `i_backtrackable`, `backtrackable_`, `tracked`, `trail` in production code
2. No shared `framed_stack<T>` / `trail<T>` generic — each component owns `std::stack<ItsFrame>`
3. `dbuct_cdcl` tests still pass unchanged
4. `./build/core_debug` all green
5. Pythagorean `dbuct-ridge` benchmark ≥ master sims/sec
6. Callgrind confirms virtual undo eliminated

---

## 11. Out of Scope (separate perf work)

| Issue | Detail |
|-------|--------|
| Frame granularity | `dbuct_sim` pushes hub frame on every MCTS choose including rollout; excess pops |
| `pop_to(depth)` | Hub could pop N frames in one call |
| Rollout region | Single transient frame for entire rollout |
| Action coalescing | Optional per-component optimization during squash/pop |

---

## 12. Key Paths

| Path | Role |
|------|------|
| `core/hpp/infrastructure/dbuct_cdcl_elimination_generator.hpp` | **Canonical pattern** — copy this shape |
| `core/hpp/value_objects/avoidance_action.hpp` | **Canonical action variant** pattern |
| `core/hpp/infrastructure/dbuct_frame_hub.hpp` | **New** — frame coordinator |
| `core/hpp/infrastructure/frame_savepoint.hpp` | **New** — replaces trail_savepoint |
| `core/hpp/infrastructure/dbuct_sim.hpp` | Hub + CDCL lockstep |
| `core/hpp/infrastructure/dbuct_ridge_manifest.hpp` | Primary wiring |
| `core/hpp/infrastructure/dbuct_mhu_elimination_generator.hpp` | Hardest migration |
| `core/hpp/infrastructure/dbuct_bind_map.hpp` | Direct logging, per-head instances |

---

## 13. Anti-Patterns — Do Not

- Introduce `framed_stack<T>`, `trail<T>`, or any shared journal base class
- Keep `tracked` "for convenience"
- Leave `make_unique` on the mutation hot path
- Use virtual methods for undo dispatch
- Centralize all action types into one repo-wide variant
- Break CDCL behavior while refactoring journals
