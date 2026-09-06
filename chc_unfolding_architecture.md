# Progressive Unfolding Database — Architecture

## Overview

The progressive unfolding database (PUD) is a passive data structure with a single public entry point:

```
unfold(subject_id, body_goal_idx, candidate_id) → coroutine<pud_observation, void>
```

It maintains the multitree described in `chc_unfolding_spec.md` and streams `unit`/`null` observations whenever candidate sets reach forced states as a side-effect of an unfold step. An external system consumes those observations and decides whether and when to issue further `unfold` calls.

The design follows the same dependency-injection-via-templates pattern used throughout `core/`. Every behavior type is a template parameterized on `I<Capability>` slots; collaborators are injected by reference at construction time. The manifest is the composition root that owns all pieces and wires them together.

---

## Package Location

All new types live under `core/`, following the established layout:

```
core/hpp/value_objects/   pud_node_id.hpp, pud_observation.hpp, ...
core/hpp/infrastructure/  pud.hpp, pud_candidacy_checker.hpp, ...
core/cpp/infrastructure/  pud.cpp, ...   (non-templated bodies)
core/test/unit/infrastructure/    pud_*_test.cpp
core/test/integration/            pud_manifest.cpp
```

All types are prefixed `pud_`.

---

## Value Objects

| Type | Contents |
|------|----------|
| `pud_node_id` | Opaque `uint32_t` identifying any node in the multitree (roots and derived nodes) |
| `pud_root_id` | Lighter opaque ID addressing only axiom roots; separate type from `pud_node_id` since the ID logic and indexing for roots is distinct |
| `pud_body_goal_idx` | Typed `uint32_t` alias for a body-goal position within a node's effective body |
| `pud_observation` | Variant: `unit(pud_node_id, pud_body_goal_idx)` or `null(pud_node_id, pud_body_goal_idx)` |
| `pud_candidate_ref` | A `pud_node_id` used as an entry in a candidate set |

---

## Root Index: `pud`

A non-templated type that manages the axiom root layer of the multitree. Knows about roots by their own lighter `pud_root_id`, and maps each to its corresponding `pud_node_id` in the node store.

**Public surface:**
- `add_root(head, body) → pud_root_id` — registers an axiom root; allocates its node in `pud_node_store` and returns the root-level ID
- `get_root(pud_root_id) → pud_node&` — returns the root node for a given root ID
- `iterate_roots()` — iterates all `pud_root_id`s; the only bulk-iteration operation exposed

---

## Node Store: `pud_node_store`

A non-templated type that owns all node memory and handles `pud_node_id` allocation. Conceptually the PUD's equivalent of `db` for rules — a flat map from `pud_node_id` to `pud_node`.

**Public surface:**
- `push_node(pud_node) → pud_node_id` — inserts a new node and returns its assigned ID
- `get_node(pud_node_id) → pud_node&` — mutable access by ID

Because nodes are stored in a flat map by ID (stable on insert), injection by reference into behavior types is safe.

---

## Infrastructure Components

Each component has exactly one job. Where possible, a single concrete object can satisfy multiple template slots.

---

### `pud_path_walker`

**Job:** given a `pud_node_id`, yields the sequence of nodes from the root ancestor down to the target (or from target up to root — direction TBD at implementation time). Provides the raw sequence of `(added_bindings, added_body_goals, unfold_body_goal_idx)` needed for path reconstruction.

**Injects:** `IGetNode` (for parent traversal)

**Called by:** `pud_head_reconstructor`, `pud_body_reconstructor`

---

### `pud_head_reconstructor`

**Job:** computes the effective head of any node by starting with `var(0)` and composing `added_bindings` at each step down the path from root to target.

**Injects:** `IWalkPath`

**Called by:** `pud_candidacy_checker`, `pud_resolver`

---

### `pud_body_reconstructor`

**Job:** computes the effective body of any node by walking its path, starting from the root's `added_body_goals` and at each subsequent step applying `added_bindings`, removing the goal at `parent.unfold_body_goal_idx`, and splicing in `node.added_body_goals`.

**Injects:** `IWalkPath`

**Called by:** `pud_resolver`

---

### `pud_candidacy_checker`

**Job:** answers "does any leaf in this node's subtree unify with goal `g`?" using the top-down, depth-first, early-pruning traversal described in §5 of the spec. Takes a `const node&` directly — no node lookup by ID is needed since the caller already has the node in hand and children are accessible directly from the node.

The traversal never reconstructs a full effective head. Instead it applies each node's `added_bindings` incrementally into a **backtrackable bind map** as it descends. If unification fails at any level, that branch is pruned and the bindings added at that level are undone via the trail before trying siblings. A leaf is found when the accumulated bindings are consistent with `g` all the way down to a node with no children.

Applies the treat-as-leaf rule when the traversal reaches the subject node `r` itself: does not recurse into `r`'s children, treating `r` as the deepest point regardless of its actual status.

This deep traversal is **only used when choosing or validating `nc`** — it is never invoked during candidate set initialization, which uses a shallow root-head check instead.

**Injects:** `IUnify` (against the accumulated bind map state at each level), `IGlobalize`

**Owns:** a `dbuct_bind_map` (the existing backtrackable bind map). The traversal calls `push_frame()` before descending into a subtree and `pop_frame()` on backtrack, which undoes all bindings added during that branch.

**Called by:** `pud_candidate_set_admitter` (for dependency-link candidates), `pud_existence_notifier` (to check the new child against watchers on leaf expansion)

---

### `pud_candidate_set_initializer`

**Job:** populates the `candidate_sets` of a newly created `pre_unfold` node `n'` (Step 4). Iterates all roots via `iterate_roots()`; for each root and each of `n'`'s body goals, performs a **shallow head unification check** — does the root's head unify with the body goal? If yes, the root reference is added to `n'.candidate_sets[g]` and the root is registered in `n'.status.candidate_existence_watchers`. No deep tree traversal is performed here; the root reference represents the whole tree as a source of candidates.

When a new axiom root is added at load time the same shallow check runs in reverse: for each existing `pre_unfold` node, each body goal is tested against the new root's head, and the root is admitted if it passes.

**Injects:** `IIterateRoots`, `IUnify`, `IAdmitCandidate`

**Called by:** `pud_unfolder` (after Step 4) and at load time (`add_root`)

---

### `pud_candidate_set_admitter`

**Job:** given a candidate node and a `(rule, goal_idx)` pair, runs the candidacy check and — if passing — inserts the candidate reference into the rule's candidate set (either `pre_unfold.candidate_sets[g]` or `mid_unfold.candidate_set`) and registers the rule in the candidate node's `candidate_existence_watchers`.

**Injects:** `ICheckCandidacy`, `IGetNode`

**Called by:** `pud_candidate_set_initializer`, `pud_wait_set_manager` (when a dependency link fires)

---

### `pud_existence_notifier`

**Job:** fires `candidate_existence_watchers` in two cases:
- **Leaf expansion (Step 4):** when node `r` gains its first child `n'`, for every `(h, g_h)` in `r.status.candidate_existence_watchers`, remove `r`'s entry from `h`'s candidate set and offer `n'` as a replacement via `pud_candidate_set_admitter`. Clear `r.status.candidate_existence_watchers`.
- **Refutation:** when a node is refuted, for every watcher, remove the entry outright. Clear the watchers.

**Injects:** `IGetNode`, `IAdmitCandidate`, `IRemoveFromCandidateSet`

**Called by:** `pud_unfolder` (end of Step 4), `pud_refutation_handler`

---

### `pud_wait_set_manager`

**Job:** two responsibilities:
1. **Registration (Step 5):** given `r` and `nc`, walk from `nc`'s root down to `nc`; for every `mid_unfold` ancestor `s` encountered, add `s` to `r.status.wait_set` and record the reverse link `r` in `s`'s watcher list.
2. **Firing:** when `r` produces a new child `n'` (Step 4), for every rule `h` that has `r` in its wait set, call `pud_candidate_set_admitter` to offer `n'` to `h`'s candidate set.

**Injects:** `IGetNode`, `IWalkPath`, `IAdmitCandidate`

**Called by:** `pud_unfolder` (Step 5, and during Step 4 to fire dependency links for the newly created child)

---

### `pud_commit`

**Job:** transitions a `pre_unfold` node to `mid_unfold` at the chosen `goal_idx` (Step 1). Moves `candidate_sets[goal_idx]` into `candidate_set`; discards other per-goal sets; carries `candidate_existence_watchers` across. Detects and returns the post-commit condition: normal, null (candidate set and wait set both empty → immediately transition to `post_unfold`), or starved (candidate set empty, wait set non-empty).

**Injects:** `IGetNode`, `ITransitionToPostUnfold`

**Called by:** `pud_unfolder` (Step 1)

---

### `pud_resolver`

**Job:** executes Steps 2–4 of the atomic unfold: rename `nc`'s variables apart (fresh frame offset), unify `r`'s committed body goal with `nc`'s effective head (always succeeds), then create and return a new child node `n'` with the resulting `added_bindings` and `added_body_goals` (unifier applied to `nc`'s effective body). Inserts `n'` into `r.children` keyed by `nc`'s node ID.

**Injects:** `IGetNode`, `IReconstructHead`, `IReconstructBody`, `IGlobalize`, `IUnify`, `INormalizeExpr`, `IPushNode`

**Called by:** `pud_unfolder` (Step 4)

---

### `pud_post_unfold_propagator`

**Job:** transitions a `mid_unfold` node to `post_unfold` and cascades (Step 9). For every rule `h` that has `r` in its wait set: removes `r` from `h.status.wait_set`; if `h`'s wait set is now empty and candidate set is also empty, transitions `h` to `post_unfold` and recurses. Discards `candidate_set` and `wait_set` from the node on transition.

**Injects:** `IGetNode`, `IIterateWaiters`

**Called by:** `pud_unfolder` (Step 8 → Step 9 when candidate set empties)

---

### `pud_unit_null_detector`

**Job:** after any mutation that changes a `pre_unfold` or `mid_unfold` node's candidate set, inspects the affected candidate sets and emits a `pud_observation` if the condition is met:
- `unit` — exactly 1 candidate, empty wait set, for any body goal of a `pre_unfold` node
- `null` — 0 candidates, empty wait set, for any body goal of a `pre_unfold` node

This component is called at every point in the unfold operation where candidate sets are mutated (after Step 1, after Step 4's watcher notifications, after Step 4's dependency link firings, after Step 7's consumption/refinement, after Step 9 cascade).

**Injects:** `IGetNode`, `IEmitObservation`

**Called by:** `pud_unfolder` at each mutation point

---

### `pud_refutation_handler`

**Job:** removes a refuted node from the multitree, fires its `candidate_existence_watchers` (via `pud_existence_notifier`) with no replacement, then propagates upward: if the refuted node's parent now has no unifying leaves in its subtree, the parent is also refuted (recursive). Triggers `pud_unit_null_detector` after each candidate set mutation that refutation causes.

**Injects:** `IGetNode`, `INotifyExistenceWatchers`, `ICheckSubtreeHasLeaf`, `IDetectUnitNull`

**Called by:** external caller (not part of `unfold`); exposed separately from the manifest

---

### `pud_unfolder`

**Job:** top-level coordinator of the atomic unfold operation. Sequences Steps 1–9 by calling into the above components. Returns a `coroutine<pud_observation, void>` that the caller can drain.

Internally, `pud_unit_null_detector` yields observations into the coroutine at each mutation point.

**Injects:** `ICommit`, `IResolve`, `IUpdateWaitSet`, `INotifyExistenceWatchers`, `IPropagatePostUnfold`, `IDetectUnitNull`, `IInitializeCandidateSets`

**Public API:**
```
coroutine<pud_observation, void> unfold(pud_node_id subject_id,
                                        pud_body_goal_idx goal_idx,
                                        pud_node_id candidate_id)
```

---

## The Manifest: `pud_manifest`

The composition root. Owns all component instances by value (following the existing manifest pattern). Wires them together via references. Exposes:
- `unfold(subject_id, goal_idx, candidate_id)` — delegates to `pud_unfolder`
- `add_root(head, body)` — delegates to `pud` + `pud_candidate_set_initializer`
- `refute(node_id)` — delegates to `pud_refutation_handler`

Constructed with any external collaborators it cannot own (e.g., `expr_pool`, `globalizer`) injected by reference.

---

## Wiring Diagram (simplified)

```
pud_manifest
 ├─ pud                            (root index: add_root, get_root, iterate_roots)
 ├─ pud_node_store                 (node storage: push_node, get_node)
 ├─ expr_pool / globalizer         (injected from outside; reused from existing infra)
 ├─ pud_path_walker                (← pud_node_store)
 ├─ pud_head_reconstructor         (← pud_path_walker)
 ├─ pud_body_reconstructor         (← pud_path_walker)
 ├─ pud_candidacy_checker          (← unifier, globalizer; owns dbuct_bind_map)
 ├─ pud_candidate_set_admitter     (← pud_candidacy_checker, pud_node_store)
 ├─ pud_candidate_set_initializer  (← pud[iterate_roots, get_root], pud_node_store,
 │                                      unifier, pud_candidate_set_admitter)
 ├─ pud_existence_notifier         (← pud_node_store, pud_candidate_set_admitter)
 ├─ pud_wait_set_manager           (← pud_node_store, pud_path_walker, pud_candidate_set_admitter)
 ├─ pud_resolver                   (← pud_node_store, pud_head_reconstructor, pud_body_reconstructor,
 │                                      globalizer, unifier, normalizer, expr_pool)
 ├─ pud_commit                     (← pud_node_store, pud_post_unfold_propagator)
 ├─ pud_post_unfold_propagator     (← pud_node_store)
 ├─ pud_unit_null_detector         (← pud_node_store)
 ├─ pud_refutation_handler         (← pud_node_store, pud_existence_notifier, pud_unit_null_detector)
 └─ pud_unfolder                   (← pud_commit, pud_resolver, pud_wait_set_manager,
                                        pud_existence_notifier, pud_candidate_set_initializer,
                                        pud_post_unfold_propagator, pud_unit_null_detector)
```

---

## Open Implementation Questions

- **Observation stream mechanism:** `coroutine<pud_observation, void>` fits the existing coroutine pattern; `pud_unit_null_detector` calls `co_yield` through an injected `IEmitObservation` slot that the coroutine frame provides.
- **Candidate set backing:** whether each candidate set is a `std::unordered_set<pud_node_id>` or a "covered" label overlay is an implementation choice deferred to the concrete type.
- **Frame offset management for rename-apart (Step 2):** the existing `globalizer` + frame offset convention can be reused directly; `pud_node_store` can track the next available frame base.
- **`pud_node` internal representation:** whether the per-status fields (`candidate_sets`, `wait_set`, etc.) are stored as a `std::variant` or as a tagged union with direct fields is a concrete implementation decision.
