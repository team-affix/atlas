# PUD Candidacy Checks

This document is the authoritative reference for how the progressive unfolding database
detects and tracks candidate existence for each body goal. It supersedes any conflicting
description of candidate sets, candidacy traversal, or unit/null detection in
`chc_unfolding_spec.md` and `chc_unfolding_architecture.md`.

---

## 1. The Database Is a Single Tree

The database is a **single tree**, not a multitree. Its root is a virtual **super-root**
whose head is `var(0)`. The super-root is pre-unfolded at load time: its children are
the axiom rules. Each axiom child's `added_unifications` contains `{ var(0) = axiom_head }`
and its `added_body_goals` contains the axiom body. All subsequent unfold steps add
children to existing nodes in the usual way.

This means the axiom rules are **nodes**, not roots. There is exactly one root in the
entire database: the super-root. "Roots" in prior documents referred to axiom nodes; they
are now uniformly axiom children of the super-root.

### Why the super-root is never a candidate

A candidate for body-goal `g` is a **leaf node** — a node with no children — whose
effective head unifies with `g`. The super-root has children (the axiom nodes) from the
moment the first axiom is loaded, so it is never a leaf and is never considered a
candidate. A body-goal `g` that would unify with `var(0)` does not accidentally acquire
the super-root as a candidate.

### Why a single tree is required for candidate counting

Tracking whether a goal has 0, 1, or 2+ candidates across the entire database requires
a traversal that sees all possible candidate nodes. With a single tree, a candidacy
traversal for body-goal `g` of rule `r` starts from the super-root and explores the
entire tree (minus `r`'s own subtree, by the treat-as-leaf rule). This gives a natural
global view without needing to enumerate multiple tree roots or merge results from
independent traversals.

---

## 2. Candidate Count Matters

The previous design tracked only **existence** (is there at least one candidate?). The
actual count is what drives forced propagation:

| Count | Condition | Scope of consequence |
|-------|-----------|----------------------|
| 0 | **Null**: no candidate for `g` anywhere in DB | Refute the entire subtree of `g`’s introducing node |
| 1 | **Unit**: exactly one candidate for `g` in DB | Every leaf-rule under `g`’s introducing node is forced to unfold against `g` |
| ≥2 | Multiple candidates exist | No forced action; defer |

The key scope: null and unit are **subgoal-level** observations. A subgoal `g` is
introduced at a specific node `N` and is implicitly shared by every descendant of `N`.
Null/unit for `g` therefore affects the entire subtree of `N`, not just one rule:

- **Null**: no leaf anywhere in the DB unifies with `g`. No descendant of `N` can ever
  be resolved. The entire subtree of `N` is refutable and can be trimmed immediately.
- **Unit**: exactly one leaf unifies with `g`. Every leaf-rule descendant of `N` is
  forced to unfold against `g` next — there is no choice, because any other ordering
  risks ending with zero candidates later.

Counting to a precise integer is not required. What is required is the ability to
distinguish 0, 1, and ≥2 — the minimum information needed to detect null and unit.

---

## 3. The 2-Watched-Literals Mechanism

Each body-goal `g` (identified by its `pud_goal_id`, which encodes the node where
`g` was introduced plus its position in `added_body_goals` — see §3a) has one
**global watch set** of at most 2 **candidate witnesses**: leaf nodes anywhere in the
entire database tree that unify with `g`. The watch set is stored in the global
`pud_goal_watch_registry` (see §6), not on any rule node.

A subgoal `g` introduced at node `N` is implicitly shared by all descendants of `N`.
The watch set for `g` does not belong to any single rule; it captures a property of the
whole subtree. Its witnesses are drawn from the whole DB — the only per-unfold exclusion
is applied at actual unfold time (the treat-as-leaf rule), not during watch tracking.

This is the direct analogue of the 2-watched-literals scheme in SAT solving:
- As long as 2 witnesses are held, `g` is provably non-unit and non-null.
- When one witness is lost, a global scan finds a replacement. If the scan fails, the
  watch set shrinks and null or unit is detected for the entire subtree of `N`.
- The mechanism does not enumerate all candidates — only enough to distinguish ≥2 from <2.

### Watch set semantics

| Watch set size | Meaning | Observation emitted |
|---|---|---|
| 2 | ≥2 candidates are known to exist | None |
| 1 | Exactly 1 candidate is currently provable | `unit(g)` — scoped to subtree of `g`’s introducing node |
| 0 | No candidates currently provable | `null(g)` — scoped to subtree of `g`’s introducing node |

A watch set of size 1 or 0 does **not** mean that no unfold or refutation has happened
yet; it means the system has searched the entire DB and found at most that many witnesses.
Unit and null are reactive: they are emitted as soon as the watch set drops below 2.

### When a witness becomes invalid

A witness `w` (a leaf node) loses validity in exactly two cases:

1. **`w` expands** — `w` gains its first child because some other rule unfolded against
   it. `w` is no longer a leaf. Every `pud_goal_id` registered in `w.candidate_existence_watchers`
   is notified.

2. **`w` is refuted** — `w` is removed from the tree. Every `pud_goal_id` registered in
   `w.candidate_existence_watchers` is notified.

On notification, the system **scans for a replacement witness**:
- If `w` expanded: search `w`'s new subtree depth-first, using the per-goal bind map for
  `g` with early pruning on unification failure. The new subtree's entries are the
  natural successors to `w` — they are specializations of `w`'s head and may still unify.
- If `w` was refuted: any remaining reachable portion of the entire DB tree is a valid
  search space. In practice, a scan starting from the nearest ancestor of `w` that
  still has unvisited subtrees is the most efficient choice.

In both cases the scan is global: it searches the whole DB, not just a subtree that
belongs to `r`.

If a replacement leaf `w'` is found: `w'` replaces `w` in the watch set; `g`'s
`pud_goal_id` is registered in `w'.candidate_existence_watchers`. The watch set size
is unchanged.

If no replacement is found: the watch set shrinks by 1. If it reaches 1, emit `unit(g)`;
the consequence is scoped to the entire subtree of `g`'s introducing node.
If it reaches 0, emit `null(g)`; the entire subtree is immediately refutable.

### Key difference from SAT 2WL

In SAT, scanning for a new watch iterates the clause's literal list — a local, bounded
operation. Here, scanning for a new witness is a **subtree traversal** bounded by the
structure of the database tree. The per-goal bind map (§4) provides early pruning, making
the scan significantly cheaper than a blind exhaustive search.


---

## 3a. Subgoal Identity: `pud_goal_id`

A **subgoal** is a goal introduced at a specific node. Its identity is the pair
`(introducing_node, position_in_added_body_goals)`, encoded as a globally unique
`pud_goal_id`.

A subgoal is created exactly once: when the node that introduces it is created (Step 4
of the unfold operation). Every descendant of the introducing node `N` implicitly carries
that subgoal in its effective body. They all share the same `pud_goal_id` — no new
identifiers are minted for inherited goals.

**Example**:

```
a :- b, c.          ← root node: added_body_goals = {b, c}
                       pud_goal_id(b) = (root, 0)
                       pud_goal_id(c) = (root, 1)

a :- d, c.          ← child: added_body_goals = {d}   (c is inherited, NOT re-introduced)
                       pud_goal_id(d) = (child, 0)
                       c still uses pud_goal_id(c) = (root, 1) from above
```

The watch set for `pud_goal_id(c) = (root, 1)` is shared by both the root and the child
node. If it drops to null, BOTH nodes (and all further descendants) are refutable.

**When is a new `pud_goal_id` allocated?** Only for goals in a node’s `added_body_goals`
at creation time — never for goals inherited from ancestors. Watch set initialization
(see §5) likewise runs only for newly introduced subgoals, not inherited ones.

---

## 3b. Variable Representation: `query_var`, `query_expr`, and the Global Query Counter

Variables in this system are not identified by `(var_index, frame_offset)` pairs tied to
a contiguous bump-allocated memory layout. Instead they use a globally unique **query
index** that carries no memory-layout semantics:

| Type | Definition | Role |
|------|-----------|------|
| `query_index` | A globally unique `uint32_t` assigned once per scope | Namespace identifier for a set of variables |
| `query_var` | `{ var_index: uint32_t, query_index }` | A specific variable within a specific scope; key type in `fp_bind_map` |
| `query_expr` | `{ expr_skeleton: const expr*, query_index }` | A term interpreted within a scope; value type in `fp_bind_map` |

`query_expr` directly replaces `framed_expr` everywhere in the system. The semantics are
identical — a skeleton expression whose `var{k}` nodes are interpreted as `query_var{k, q}`
— but `query_index` is an opaque counter value rather than a stride-based frame address.

**The global query counter** is a single program-wide monotonically-increasing integer.
Every time a new scope is needed — when a new DB node is created, when a candidacy
traversal initiates, or when a candidacy traversal visits a DB node — one or more fresh
`query_index` values are consumed from this counter. The counter never resets and values
are never reused.

**Why monotonically global, not hierarchical**: different leaf rules at the same tree
depth independently increment the same counter when they start candidacy queries. If the
counter were per-node or per-subtree, two independent queries could both produce
`query_index = 4`, causing variable collisions when one query's traversal encounters
bindings left by the other. A single global counter guarantees uniqueness regardless of
how many simultaneous or nested queries are active.

---

## 4. Per-Goal Bind Maps for Candidacy Traversal

Each body-goal `g` (addressed by its global `pud_goal_id`) has its own `fp_bind_map`
instance, keyed by `query_var` and valued by `query_expr`. This map stores the cumulative
bindings accumulated as candidacy traversals for `g` descend through nodes.

**Initialization**: when a candidacy query for subgoal `g` is initiated from leaf rule
`L`, the per-goal bind map starts with **L's complete binding environment** — all
`query_var → query_expr` entries visible at `L`'s Euler-tour position in the base bind
map — plus one initial binding for the new query:

```
(var_index=0, query_index=Q_new) = g_as_query_expr
```

`Q_new` is a fresh `query_index` consumed from the global counter specifically for this
query's "head slot". This binding says: the head that candidates must unify with is `g`
(as seen from `L`'s binding context).

**Fresh query_index per visited node**: when the traversal descends to DB node `D`, a
fresh `query_index` `Q_D_cand` is consumed from the global counter for `D`'s variables in
this traversal. `D`'s `added_unifications` are read with their stored `query_index` values
remapped to `Q_D_cand`. The resulting `query_var{k, Q_D_cand}` keys are distinct from
`D`'s original `query_var{k, Q_D_original}` keys already present in `L`'s inherited
context — they coexist in the same map without collision. Ancestor bindings (recorded at
ancestor nodes' Euler-tour labels) propagate to descendant positions via the standard
predecessor-search mechanism.

**Lazy propagation**: entries are recorded in `g`'s map only when a traversal for `g`
actually visits a node. Nodes not yet visited by any traversal for `g` have no entry.
On the next traversal, previously visited nodes are reused; newly visited nodes have their
bindings recorded on first access.

**Persistence across branches**: `fp_bind_map` uses Euler-tour interval labeling, so
bindings recorded for one branch of the tree are invisible when querying from a different
branch. A traversal that descends into a subtree and fails unification requires no explicit
undo — the failed subtree's entries are scoped to their interval and do not affect queries
from sibling or ancestor positions.

**Traversal invariant**: before querying unification at node `n` on behalf of goal `g`,
all ancestors of `n` must have entries in `g`'s map. This is guaranteed by the top-down
traversal order: a node is only visited after its parent is visited and its parent's
bindings are recorded.

**Rescan reuse**: when a witness `w` is lost and a rescan begins, `g`'s map already
contains entries for the path from the super-root down to `w`'s parent. The rescan
starts from `w`'s subtree entry point and can immediately exploit those existing entries
without re-traversal of the path above.

---

## 5. Initial Watch Set Population

When a new `pre_unfold` node `n'` is created (Step 4 of the unfold operation), watch
sets are initialized **only for the subgoals introduced at `n'`** — that is, only for
the goals in `n'.added_body_goals`. Goals inherited from ancestors already have their
`pud_goal_id`s and watch sets from when they were first introduced; no new watch set
is created for them.

**Procedure**: for each goal `g` at position `i` in `n'.added_body_goals`:
1. Allocate a fresh `pud_goal_id = (n', i)` and create an empty entry in
   `pud_goal_watch_registry`.
2. Perform a candidacy traversal from the super-root, seeking at most 2 unifying leaves.
   Stop as soon as 2 witnesses are in hand — do not continue enumerating all candidates.
   (The treat-as-leaf rule applies at actual unfold time, not here.)
3. For each found witness `w`: add `w` to the registry entry for `g`'s `pud_goal_id`
   and register `g`'s `pud_goal_id` in `w.candidate_existence_watchers`.
4. If fewer than 2 witnesses were found: the corresponding null or unit condition holds
   for the entire subtree of `n'` — emit the observation immediately.

The traversal uses `g`'s newly created (empty) per-goal bind map (see §4). Because no
prior traversal has visited any node on behalf of this `pud_goal_id`, all bindings are
recorded fresh during this population pass.

**At `add_root` time**: when a new axiom node is added to the super-root, its
`added_body_goals` introduce new subgoals. A watch-set population pass runs for each.
Additionally, the new axiom node is itself a new leaf — any existing watch set that is
below capacity 2 is rescanned to check whether this new leaf is a valid replacement
witness for its goal.

---

## 6. Watch Set Storage

The watch set for a subgoal `g` is stored globally in the **goal-watch registry** —
a map from `pud_goal_id` to a fixed-capacity set of ≤2 `pud_node_id` witnesses. This
registry is its own infrastructure component (`pud_goal_watch_registry`).

Because a subgoal is introduced once and shared across all descendants of its
introducing node, there is exactly one registry entry per subgoal — not one per
descendant rule. All descendants consult the same entry; a change to the watch set
immediately affects all of them.

The **reverse index** — mapping each leaf node `n` to the set of subgoals currently
watching it — is stored on `n` itself:

| Field on node status | Meaning |
|----------------------|---------|
| `candidate_existence_watchers` | The set of `pud_goal_id`s whose global watch set currently contains this node; fired and cleared when the node expands or is refuted |

`pre_unfold` nodes carry **no** watch-set field. The watch set lives entirely in the
registry; only the reverse index lives on the node.

---

## 7. Interaction with `mid_unfold`

When a `pre_unfold` node `r` commits to `mid_unfold` at body-goal `g` (Step 1):

- The ≤2 witnesses in the global registry entry for `g`'s `pud_goal_id` are moved into
  `r.status.candidate_set` as the initial seed.
- `pud_goal_watch_registry` entries for `r`'s OTHER body goals are deleted (the subgoals
  those goals represent are still live for siblings and ancestors of `r` in the tree, so
  only `r`'s local reference to them is released, not the registry entries themselves if
  other descendants of the same introducing node still exist).
- `r.candidate_existence_watchers` is carried across the transition — `r` is still a
  leaf until Step 4 creates the first child.

The 2WL mechanism is a `pre_unfold` concern. Once a node commits to `mid_unfold`, its
candidate set is managed directly; no watch-set replacement scanning occurs for it.
However, the watch sets for the INHERITED subgoals of `r`'s ancestors remain active:
a null or unit detection for an ancestor's subgoal still affects `r` (via the subtree
scope rule in §2).

---

## 8. Unit and Null Detection Points

Whenever a watch set drops below 2, the detection fires. The observation and its
consequence are scoped to the **entire subtree** of the subgoal’s introducing node
(see §2).

| Event | What triggers detection |
|-------|-------------------------|
| Watch set population (§5) | < 2 witnesses found for any newly introduced subgoal |
| Witness expansion | Rescan of `w`'s new subtree fails to find a replacement |
| Witness refutation | Rescan of remaining DB fails to find a replacement |
| `mid_unfold` commit (Step 1) | Candidate set starts at 0 or 1 entries |
| Dependency link fires | One entry added to a previously empty `mid_unfold.candidate_set` |

A null detection for subgoal `g` (introduced at node `N`) means the entire subtree of
`N` is immediately refutable: call `refute(N)` (which propagates downward to all
descendants). A unit detection means all `pre_unfold` leaves under `N` should commit
to unfolding against `g` before any other body goal.

---

## 9. Sections Superseded in Other Documents

The following sections of `chc_unfolding_spec.md` are superseded by this document:

- **§1 "Database State"**: the description of a multitree with multiple root nodes is
  replaced by the single-tree design with a var(0) super-root (§1 above).
- **`pre_unfold.candidate_sets`**: the per-goal vector of live candidate sets is replaced
  by the global goal-watch registry (§6 above); the watch set is no longer stored on the
  rule node itself.
- **§4 "Candidate Sets"**: the full-enumeration candidate set description and its live
  additions, dependency link, and leaf expansion propagation sub-sections are superseded;
  the 2WL mechanism and its watch invalidation handling (§3 above) take their place.

The following sections of `chc_unfolding_architecture.md` are superseded:

- **`pud` root index**: `iterate_roots()` no longer exists; the single tree's super-root
  is a fixed node; axiom-loading via `add_root` is retained (§1 above).
- **`pud_candidate_set_initializer`**: replaced by `pud_watch_set_initializer`, which
  runs a traversal seeking ≤ 2 witnesses rather than enumerating all candidate roots
  (§5 above).
- **`pud_unit_null_detector`**: now inspects watch set sizes (0 and 1) on `pre_unfold`
  nodes rather than full candidate set counts (§8 above).
