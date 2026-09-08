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
| `pud_goal_id` | Globally unique identifier for a `(introducing_node, position_in_added_body_goals)` pair — used to address per-goal candidacy binding tables and the goal-watch registry |
| `pud_observation` | Variant: `unit(pud_node_id, pud_body_goal_idx)` or `null(pud_node_id, pud_body_goal_idx)` |
| `pud_candidate_ref` | A `pud_node_id` used as an entry in a candidate set |
| `query_index` | A globally unique `uint32_t` issued by the global query counter; identifies a variable scope. Never reused. See `pud-candidacy-checks.md` §3b |
| `query_var` | `{ var_index: uint32_t, query_index }` — a specific variable within a specific scope; the key type for all `fp_bind_map` instances in PUD |
| `query_expr` | `{ expr_skeleton: const expr*, query_index }` — a term interpreted within a scope; the value type for all `fp_bind_map` instances in PUD; replaces `framed_expr` |

---

## Root Index: `pud`

A non-templated type that manages the single database tree. The tree has one fixed
super-root (head = `var(0)`); axiom rules are its direct children. `pud` maps each
axiom's lighter `pud_root_id` to its corresponding `pud_node_id` in the node store.

**Public surface:**
- `add_root(head, body) → pud_root_id` — registers an axiom rule as a child of the super-root; allocates its node in `pud_node_store` with `added_unifications = { var(0) = head }`
- `get_root(pud_root_id) → pud_node&` — returns the axiom node for a given root ID

`iterate_roots()` is removed: the single-tree design requires no bulk root iteration.
The candidacy traversal starts from the super-root and descends uniformly.

---

## Node Store: `pud_node_store`

A non-templated type that owns all node memory and handles `pud_node_id` allocation. Conceptually the PUD's equivalent of `db` for rules — a flat map from `pud_node_id` to `pud_node`.

**Public surface:**
- `push_node(pud_node) → pud_node_id` — inserts a new node and returns its assigned ID
- `get_node(pud_node_id) → pud_node&` — mutable access by ID

Because nodes are stored in a flat map by ID (stable on insert), injection by reference into behavior types is safe.

---

## Fully Persistent Bind Map: `fp_bind_map`

`fp_bind_map` is a generic, reusable type that is not specific to PUD. It lives at `core/hpp/infrastructure/fp_bind_map.hpp`.

It implements tree-shaped binding environments with multiple simultaneously-active branches, using:
- **Euler-tour interval labeling** (Dietz–Sleator order maintenance): each node is assigned `open` / `close` labels at creation time in O(1) amortized; `u` is an ancestor of `v` iff `open(u) < open(v) < close(u)`.
- **Per-variable predecessor timelines**: when a variable `x` is bound to term `t` at node `n`, two events are recorded — value `t` at `open(n)` and the prior shadowed value at `close(n)`. A lookup for `x` at any node `v` is a single predecessor query for `open(v)` in `x`'s timeline.

This gives O(1) amortized record, O(log log n) lookup (or O(log k) where `k` is the number of times that variable was bound), and O(1) space per binding — no quadratic blowup from storing full bind maps at every node.

**Key / value types:** `query_var` as key, `query_expr` as value (see Value Objects above).

**Public surface:** `record(open, close, query_var, query_expr)`, `query(open, query_var) → query_expr`

---

## Binding Tables in PUD

PUD uses `fp_bind_map` in two roles:

**Base bind map** — one instance, shared across the entire multitree. When node `n` is created, its `added_unifications` are solved in the context of its ancestors' bindings (queried from this map at `n`'s parent labels) and the resulting concrete variable bindings are recorded at `open(n)` / `close(n)`. Used for path reconstruction (effective head and body) and for the unfold operation itself — verifying that `r.body[g]` unifies with `nc`'s effective head.

**Per-goal bind maps** — one `fp_bind_map` instance per tracked goal (keyed by `pud_goal_id`). Each stores only the additional bindings introduced when a candidacy check for that goal descends through a node, layered on top of the base map (goal-specific bindings shadow base bindings). Populated lazily — only when a candidacy check for the corresponding goal actually visits a node. Backtracking on failed candidacy branches requires no explicit undo: each branch's events are scoped to their Euler-tour interval and become invisible outside it.

Allocation and lifecycle of per-goal maps is an open implementation question (pre-allocated map keyed by `pud_goal_id` vs. on-demand creation at first candidacy check).

---

## Infrastructure Components

Each component has exactly one job. Where possible, a single concrete object can satisfy multiple template slots.

---

### `pud_path_walker`

**Job:** given a `pud_node_id`, yields the sequence of nodes from the root ancestor down to the target (or from target up to root — direction TBD at implementation time). Provides the raw sequence of `(added_unifications, added_body_goals, unfold_body_goal_idx)` needed for path reconstruction.

**Injects:** `IGetNode` (for parent traversal)

**Called by:** `pud_head_reconstructor`, `pud_body_reconstructor`

---

### `pud_head_reconstructor`

**Job:** computes the effective head of any node by querying `var(0)` from the base binding table at the target node's Euler-tour labels (a single predecessor lookup returns the most recent binding of `var(0)` on the path root → target).

**Injects:** `IQueryBaseBindMap`

**Called by:** `pud_resolver`

---

### `pud_body_reconstructor`

**Job:** computes the effective body of any node by walking its path, starting from the root's `added_body_goals` and at each subsequent step removing the goal at `parent.unfold_body_goal_idx`, splicing in `node.added_body_goals`, then resolving variables in the running goal list via the base binding table at that node's labels.

**Injects:** `IWalkPath`, `IQueryBaseBindMap`

**Called by:** `pud_resolver`

---

### `pud_candidacy_checker`

**Job:** answers "does any leaf in this node's subtree unify with goal `g` (identified by `pud_goal_id`)?" using the top-down, depth-first, early-pruning traversal described in §5 of the spec. Takes a `const node&` directly.

The traversal uses the **per-goal candidacy binding table** for `g`. At each node `n` visited, it checks whether `g`'s table already covers `n` (lazy propagation from a prior check). If not, it solves `n.added_unifications` in the combined context of the base binding table and `g`'s table at the parent's labels, records the new bindings in `g`'s table at `open(n)` / `close(n)`, then attempts unification with `g`. If unification fails, the subtree is pruned (descendants are strictly more specialized and cannot succeed). If it succeeds and `n` is a leaf, the check returns true. If it succeeds and `n` is internal, the traversal recurses into children.

Because the per-goal table is fully persistent, backtracking on failed branches requires no explicit undo — the table retains prior state through its predecessor timeline. Each branch's entries are scoped to their Euler-tour interval and become invisible outside of it.

Applies the treat-as-leaf rule when the traversal reaches the subject node `r` itself: does not recurse into `r`'s children.

This deep traversal is **only used when choosing or validating `nc`** — never during candidate set initialization, which uses a shallow root-head check.

**Injects:** `IUnify`, `IQueryBaseBindMap`, `IExtendGoalBindMap`

**Called by:** `pud_candidate_set_admitter` (for dependency-link candidates), `pud_existence_notifier` (to check the new child against watchers on leaf expansion)

---

### `pud_goal_watch_registry`

**Job:** the global store of watch sets. Maintains a map from `pud_goal_id` to a
fixed-capacity set of ≤2 `pud_node_id` witnesses. Is the single source of truth for
whether a body-goal currently has 0, 1, or ≥2 known candidate witnesses in the whole DB.

**Public surface:**
- `add_watch(pud_goal_id, pud_node_id)` — inserts a witness; errors if already at capacity 2
- `remove_watch(pud_goal_id, pud_node_id)` — removes a witness; returns the new watch count
- `watch_count(pud_goal_id) → uint8_t` — returns 0, 1, or 2
- `delete_goal(pud_goal_id)` — removes the entire entry (called at commit time for other goals)

**Does not** store per-rule information. Two rules with different `pud_goal_id`s have
independent entries even if their goals are structurally identical.

**Injects:** nothing (pure data store)

**Called by:** `pud_watch_set_initializer`, `pud_existence_notifier`, `pud_unit_null_detector`, `pud_commit`

---

### `pud_watch_set_initializer`

**Job:** populates the global watch set for each body-goal `g` of a newly created
`pre_unfold` node `n'` (Step 4). For each body goal `g` of `n'`, allocates a fresh
`pud_goal_id`, then performs a **depth-first candidacy traversal** of the database tree
(excluding `n'`'s own subtree, by the treat-as-leaf rule), seeking at most 2 unifying
leaves. Each found leaf witness `w` is registered in `pud_goal_watch_registry` under
`g`'s `pud_goal_id`, and `g`'s `pud_goal_id` is added to `w.candidate_existence_watchers`.
The traversal stops as soon as 2 witnesses are in hand. If fewer than 2 are found, the
appropriate `unit` or `null` observation is emitted immediately.

When a new axiom node is added at load time the same mechanism runs in the other
direction: a rescan is triggered for every `pre_unfold` node whose watch set for any
body goal is below 2, checking whether the new axiom node is a valid replacement witness.

See `pud-candidacy-checks.md` §5 for the full specification.

**Injects:** `IUnify`, `IGetNode`, `IUpdateWatchRegistry`, `IEmitObservation`

**Called by:** `pud_unfolder` (after Step 4) and at load time (`add_root`)

---

### `pud_candidate_set_admitter`

**Job:** given a candidate node and a `(rule, goal_idx)` pair, runs the candidacy check
and — if passing — inserts the candidate reference into the rule's candidate set
(`mid_unfold.candidate_set`) and registers the rule in the candidate node's
`candidate_existence_watchers`. This component handles the `mid_unfold` case; watch-set
admission for `pre_unfold` nodes is handled by `pud_watch_set_initializer`.

**Injects:** `ICheckCandidacy`, `IGetNode`

**Called by:** `pud_wait_set_manager` (when a dependency link fires)

---

### `pud_existence_notifier`

**Job:** fires `candidate_existence_watchers` in two cases:
- **Leaf expansion (Step 4):** when node `r` gains its first child `n'`, for every `pud_goal_id g` in `r.candidate_existence_watchers`, remove `r` from `g`'s watch set in the registry and scan `r`'s new subtree for a replacement witness. Clear `r.candidate_existence_watchers`.
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

**Job:** transitions a `pre_unfold` node to `mid_unfold` at the chosen `goal_idx` (Step 1). Seeds `candidate_set` from `watches[goal_idx]` (≤2 witnesses); discards watch sets for other body goals; carries `candidate_existence_watchers` across. Detects and returns the post-commit condition: normal, null (candidate set and wait set both empty → immediately transition to `post_unfold`), or starved (candidate set empty, wait set non-empty).

**Injects:** `IGetNode`, `ITransitionToPostUnfold`

**Called by:** `pud_unfolder` (Step 1)

---

### `pud_resolver`

**Job:** executes Steps 2–4 of the atomic unfold: allocate a fresh `query_index` for `nc`'s variables (rename apart), unify `r`'s committed body goal with `nc`'s effective head (always succeeds — see spec §6 Step 3), then create a new child node `n'`:
- `n'.added_unifications` = the unification equations from Step 3, expressed as `query_expr` pairs using `nc`'s fresh `query_index`
- `n'.added_body_goals` = terms from `nc`'s effective body that replace `r.body[g]`
- `n'.status` = `pre_unfold`; `n'.children` = `{}`

Assigns Euler-tour labels `open(n')` / `close(n')`. Solves `n'.added_unifications` in the combined context of the base binding table and the per-goal bind map at `r`'s labels (accepting the candidate's accumulated binding environment), and records the concrete bindings in the base binding table at `n'`'s labels. Inserts `n'` into `r.children` keyed by `nc`'s node ID.

**Injects:** `IGetNode`, `IReconstructHead`, `IReconstructBody`, `IAllocateQueryIndex`, `IUnify`, `INormalizeExpr`, `IPushNode`, `IAssignLabels`, `IExtendBaseBindMap`, `IQueryGoalBindMap`

**Called by:** `pud_unfolder` (Step 4)

---

### `pud_post_unfold_propagator`

**Job:** transitions a `mid_unfold` node to `post_unfold` and cascades (Step 9). For every rule `h` that has `r` in its wait set: removes `r` from `h.status.wait_set`; if `h`'s wait set is now empty and candidate set is also empty, transitions `h` to `post_unfold` and recurses. Discards `candidate_set` and `wait_set` from the node on transition.

**Injects:** `IGetNode`, `IIterateWaiters`

**Called by:** `pud_unfolder` (Step 8 → Step 9 when candidate set empties)

---

### `pud_unit_null_detector`

**Job:** after any mutation that changes a node's watch set (`pre_unfold`) or candidate
set (`mid_unfold`), inspects the affected set and emits a `pud_observation` when:
- `unit` — watch set size == 1 (exactly one witness known), for any body goal of a `pre_unfold` node
- `null` — watch set size == 0 (no witnesses), for any body goal of a `pre_unfold` node

Also fires on `mid_unfold` candidate set transitions (0 or 1 entries with empty wait set).

Called at every mutation point: after watch-set population (Step 4 / `add_root`), after
watch invalidation rescans, after Step 1 commit, and after Step 9 cascade.

See `pud-candidacy-checks.md` §8 for the full list of detection points.

**Injects:** `IGetNode`, `IEmitObservation`

**Called by:** `pud_unfolder`, `pud_watch_set_initializer`, `pud_existence_notifier` at each mutation point

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
- `add_root(head, body)` — delegates to `pud` + `pud_watch_set_initializer`
- `refute(node_id)` — delegates to `pud_refutation_handler`

Constructed with any external collaborators it cannot own (e.g., `expr_pool`) injected by reference.

---

## Wiring Diagram (simplified)

```
pud_manifest
 ├─ pud                            (single-tree root index: add_root, get_root)
 ├─ pud_node_store                 (node storage: push_node, get_node)
 ├─ expr_pool                       (injected from outside; reused from existing infra)
 ├─ pud_query_counter              (global query_index allocator; owns the monotonic counter)
 ├─ pud_path_walker                (← pud_node_store)
 ├─ pud_head_reconstructor         (← pud_path_walker)
 ├─ pud_body_reconstructor         (← pud_path_walker)
 ├─ pud_candidacy_checker          (← unifier, pud_query_counter; owns per-goal fp_bind_maps)
 ├─ pud_candidate_set_admitter     (← pud_candidacy_checker, pud_node_store)
 ├─ pud_goal_watch_registry          (watch sets keyed by pud_goal_id)
 ├─ pud_watch_set_initializer      (← pud_node_store, unifier, pud_goal_watch_registry, pud_unit_null_detector)
 ├─ pud_wait_set_manager           (← pud_node_store, pud_path_walker, pud_candidate_set_admitter)
 ├─ pud_resolver                   (← pud_node_store, pud_head_reconstructor, pud_body_reconstructor,
 │                                      pud_query_counter, unifier, normalizer, expr_pool)
 ├─ pud_commit                     (← pud_node_store, pud_post_unfold_propagator)
 ├─ pud_post_unfold_propagator     (← pud_node_store)
 ├─ pud_unit_null_detector         (← pud_node_store)
 ├─ pud_refutation_handler         (← pud_node_store, pud_existence_notifier, pud_unit_null_detector)
 └─ pud_unfolder                   (← pud_commit, pud_resolver, pud_wait_set_manager,
                                        pud_existence_notifier, pud_watch_set_initializer,
                                        pud_post_unfold_propagator, pud_unit_null_detector)
```

---

## Open Implementation Questions

- **Observation stream mechanism:** `coroutine<pud_observation, void>` fits the existing coroutine pattern; `pud_unit_null_detector` calls `co_yield` through an injected `IEmitObservation` slot that the coroutine frame provides.
- **Candidate set backing:** whether each candidate set is a `std::unordered_set<pud_node_id>` or a "covered" label overlay is an implementation choice deferred to the concrete type.
- **Global query counter:** a single program-wide monotonically-increasing `uint32_t` counter issues all `query_index` values — for new DB nodes at creation time, for candidacy query head-slot bindings, and for each DB node visited during a candidacy traversal (rename-apart, Step 2). The counter is owned by the manifest or a dedicated `pud_query_counter` component and injected via `IAllocateQueryIndex` wherever a fresh `query_index` is needed. It never resets.
- **`pud_node` internal representation:** whether the per-status fields (`watches`, `wait_set`, etc.) are stored as a `std::variant` or as a tagged union with direct fields is a concrete implementation decision.
