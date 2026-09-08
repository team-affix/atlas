# Progressive Database Unfolding — Specification

## 1. Database State

The database D is a **single tree**. Its root is a virtual super-root with head `var(0)`.
Axiom rules are the super-root's direct children (loaded via `add_root`); every derived
node is produced by exactly one unfold step. There are no separate "tree" or "edge"
objects — the database is simply the set of all nodes. Every non-root node's effective
head is a **specialization** of its root ancestor's head (specialization = strictly
additive instantiation; variable bindings are only added, never removed).

> **Candidacy tracking** (how the database detects unit/null for body goals, the
> 2-watched-literals watch sets, and per-goal bind maps) is fully specified in
> `pud-candidacy-checks.md`, which is the authoritative reference for those topics.

---

## 2. Structure: Nodes

There is one type: **node**.

| Field | Meaning |
|-------|---------|
| `status` | `pre_unfold`, `mid_unfold`, or `post_unfold` (see §3) |
| `added_unifications` | The unification equations introduced at this node: for a root node, `{var(0) = axiom_head}`; for a derived node, the set of term-pairs that were unified during the unfold step that produced it |
| `added_body_goals` | For a root node: the axiom body; for a derived node: the terms from `nc`'s effective body introduced to replace the unified goal |
| `children` | A map from candidate rule ID to child node — each child is keyed by the ID of the candidate rule that was unfolded against to produce it |

`var(0)` is a reserved variable index serving as the universal head slot across the entire multitree.

The super-root is the only node with no parent. All axiom nodes and derived nodes are reachable from it.

A node with no children is a **leaf** — an implicit property with no maintained set; a node is a leaf simply because `children` is empty.

### Variable Representation

Variables are identified by `query_var = { var_index, query_index }` where `query_index`
is a globally unique, monotonically increasing integer assigned from a single program-wide
counter. Terms are represented as `query_expr = { expr_skeleton, query_index }`. Both
replace the older `(var_index, frame_offset)` / `framed_expr` scheme; `query_index`
carries no memory-layout semantics and can be freely allocated without stride constraints.
See `pud-candidacy-checks.md` §3b for the full definition.

### Binding Tables

Variable bindings are not stored on nodes. Instead, the system uses `fp_bind_map` — a
fully persistent bind map keyed by `query_var` and valued by `query_expr`, using
Euler-tour interval labeling and per-variable predecessor timelines (see the
fully-persistent-binding-tree design notes and the architecture doc). Each node is
assigned an `open` and `close` label at creation time.

**Base bind map:** one `fp_bind_map` instance shared by the entire tree. When node `n`
is created, `n.added_unifications` are solved in the context of all ancestor bindings
(queried at `n`'s parent labels) and the results are recorded at `open(n)` / `close(n)`.
Used during the unfold operation itself — to verify that unification of `r`'s body goal
with `nc`'s effective head succeeds — and for path reconstruction of effective heads and
bodies.

**Per-goal bind maps:** one additional `fp_bind_map` per tracked goal, identified by a
global `pud_goal_id`. Initialized with the querying leaf rule's full binding environment
plus the initial head-slot binding; populated lazily thereafter during candidacy
traversals. See `pud-candidacy-checks.md` §4 for the full description.

### Status Variants

The `pre_unfold` status carries:

| Field | Meaning |
|-------|---------|
| `candidate_existence_watchers` | The set of `pud_goal_id`s whose global watch set (in the goal-watch registry) currently contains this node as a witness; preserved through the `pre_unfold → mid_unfold` transition; fired and cleared when the node gains its first child or is refuted |

`pre_unfold` carries no watch-set field. Watch sets are stored in the global
`pud_goal_watch_registry` keyed by `pud_goal_id`; see `pud-candidacy-checks.md` §6.

The `mid_unfold` status carries:

| Field | Meaning |
|-------|---------|
| `unfold_body_goal_idx` | The body-goal index committed to for this unfold |
| `candidate_set` | Node references remaining to be consumed; no descendant of this node may appear in it |
| `wait_set` | Nodes this node is waiting on to deliver future candidates (see §3 Wait Set) |
| `candidate_existence_watchers` | Carried over from `pre_unfold`; fired and cleared in Step 4 when the first child is created, or on refutation; empty for all subsequent children |

The `post_unfold` status carries:

| Field | Meaning |
|-------|---------|
| `unfold_body_goal_idx` | Retained from the `mid_unfold` phase for path reconstruction |

`candidate_set` and `wait_set` are discarded on transition to `post_unfold`.

### Path Reconstruction

Both the effective head and full body of any node `n` are recovered by walking the path from the root ancestor down to `n`, using the base binding table for variable resolution.

**Effective head**: look up `var(0)` in the base binding table at `n`'s labels (the predecessor query answers with the most recent binding of `var(0)` on the path root → `n`). This immediately yields the axiom head after applying accumulated specializations.

**Full body**: start with the root node's `added_body_goals` (the axiom body). At each subsequent node on the path, remove the goal at `parent.status.unfold_body_goal_idx`, splice in `node.added_body_goals`, then resolve any variables in the current body list via the base binding table at that node's labels. `parent.status.unfold_body_goal_idx` is always available since it is a field on both `mid_unfold` and `post_unfold` status variants.

---

## 3. Node Status

| Status | Children | Meaning |
|--------|----------|---------|
| `pre_unfold` | 0 | Leaf; no unfolding decision made; global watch sets for its body goals live in `pud_goal_watch_registry` (2WL mechanism) |
| `mid_unfold` | ≥ 1 | Committed to `unfold_body_goal_idx`; children represent already-consumed candidates; `candidate_set` holds remaining ones |
| `post_unfold` | ≥ 0 | Candidate set and wait set exhausted; permanently done as a subject of unfolding |

**Starved** is a derived condition, not a stored state: a node is starved when `status == mid_unfold && candidate_set.empty() && !wait_set.empty()`. The `mid_unfold` status covers both the active-candidates case and the temporarily-starved case.

**Facts** (nodes with no body goals) stay `pre_unfold` permanently with empty `watches` — there is no body-goal to commit `unfold_body_goal_idx` to. Whether a node is a fact is implicit from its `added_body_goals` being empty.

**Critical distinction — subject vs target:**

- **Subject of unfolding** (the rule `r` being unfolded): must be `pre_unfold` or `mid_unfold`; these form the active cut
- **Target of unfolding** (the `nc` another rule resolves against): any node — `pre_unfold`, `mid_unfold`, or `post_unfold`

`post_unfold` nodes leave the active cut only as subjects. They remain fully reachable as targets, which is what allows shallow unfolds over fully-unfolded axioms.

### Wait Set

The `wait_set` inside an `mid_unfold` node `r` is a set of `mid_unfold` nodes. Each entry `s ∈ wait_set(r)` means:

- `r` descended below `s` when choosing `nc`, and therefore committed to covering all of `s`'s future children as part of `r`'s own cut
- Whenever `s` produces a new child `s_i`, `s_i` is offered to `r.status.candidate_set` subject to a candidacy check against `r`'s committed body-goal; if it passes it is admitted, transitioning `r` out of the starved condition
- When `s` becomes `post_unfold`, `s` is removed from `wait_set(r)`; if `wait_set(r)` is then empty and `candidate_set` is still empty, `r` transitions to `post_unfold`

**Which ancestors enter the wait set:** when `r` unfolds against `nc` and `nc` is a descendant of an `mid_unfold` node `s` in `nc`'s tree, every `mid_unfold` ancestor of `nc` along the path from `nc`'s root to `nc` is added to `wait_set(r)`. If `nc` is chosen directly as an `mid_unfold` node (not a descendant of one), no wait set entry is created — the result covers `nc`'s current state completely and further specialization is deferred to future unfolding of the resulting child.

---

## 4. Candidate Watching

> **This section is fully specified in `pud-candidacy-checks.md`**, which is the
> authoritative reference. The summary below is for orientation only.

Each body-goal `g` (identified by `pud_goal_id`) has a **global watch set** of at most
2 candidate witnesses — leaf nodes anywhere in the whole DB whose effective head unifies
with `g`. Tracking 2 witnesses is sufficient to distinguish the three cases that matter:
0 candidates (null), 1 candidate (unit), and ≥2 candidates (deferred). Watch sets are
stored in a global `pud_goal_watch_registry`, not on the rule node.

When `r` commits to unfolding at `g`, the ≤2 witnesses in the registry entry for `g` seed
`r.status.candidate_set` in the new `mid_unfold` status; registry entries for other body
goals are deleted.
The `mid_unfold.candidate_set` then grows through dependency link firings from the wait
set (see §3 Wait Set) and is consumed one entry at a time as unfold steps proceed.

**Watch invalidation:** when a watched leaf `n` expands (gains its first child) or is
refuted, every `(h, g_h)` in `n.candidate_existence_watchers` scans for a replacement
witness. If the scan finds one, the watch set is restored to full size. If not, the
watch set shrinks and the appropriate `unit` or `null` observation is emitted.
See `pud-candidacy-checks.md` §3 for the full mechanism.

---

## 5. Candidacy

A node `n` is a **candidate for body-goal `g`** iff at least one leaf in `n`'s subtree unifies with `g`.

**Candidacy traversal** (top-down, depth-first, exploiting the per-goal binding table):

- At node `n`: attempt to extend goal `g`'s binding table to cover `n` — solve `n.added_unifications` in the combined context of the base table and `g`'s table at `n`'s ancestor labels, then try to unify with `g`. If the goal's table already has an entry covering `n` (lazy propagation reached this node earlier), reuse it directly.
- If unification **fails**: prune the entire subtree. Every descendant is a stricter specialization, so descendants cannot unify if the ancestor does not.
- If unification **succeeds** and `n` is a **leaf**: `n` is a candidate; stop immediately.
- If unification **succeeds** and `n` is **internal**: recurse into children.

**Self-candidacy and the treat-as-leaf rule:** when building `r`'s candidate set and the traversal reaches `r` itself, `r` is treated as a leaf regardless of its actual status — the traversal does not recurse into `r`'s children. If `r`'s head unifies with the goal, `r` is added as a candidate for itself; if not, `r`'s subtree is pruned. Either way, `r`'s proper descendants are never reachable by the traversal and therefore never enter `r`'s candidate set. This is the structural enforcement of the no-descendant invariant.

This is an **existence check**, not enumeration. The traversal halts at the first unifying leaf. In the worst case the entire subtree is traversed to disprove candidacy; early subtree pruning is the primary source of speed-up from unfolding.

---

## 6. The Atomic Unfold Operation

The operation takes as input:

- `r`: the rule to unfold (a `pre_unfold` or `mid_unfold` node)
- `g`: the body-goal index (required if `r` is pre_unfold; must match `r.status.unfold_body_goal_idx` if mid_unfold)
- `nc`: the candidate node to unfold against (any node — `pre_unfold`, `mid_unfold`, or `post_unfold`)

and produces at most one new node.

**Step 1 — Commit (only if `r` is pre_unfold)**

Transition `r` to `mid_unfold(g, candidate_set, wait_set={})` where `candidate_set` is seeded from the ≤2 witnesses in the global `pud_goal_watch_registry` entry for `g`. Registry entries for `r`'s other body goals are deleted. From this point, descendants of `r` are excluded from `candidate_set` by the treat-as-leaf rule; dependency-link firings from the wait set may still add entries.

*Null propagation:* if `candidate_set` is empty and `wait_set` is empty, transition `r` to `post_unfold` immediately and produce no child.

*Starved at commit:* if `candidate_set` is empty but `wait_set` is non-empty, `r` enters the mid_unfold state in the starved condition — no child produced until a dependency fires.

*Unit propagation:* if `|candidate_set| = 1` and `wait_set` is empty, the following steps are forced.

**Step 2 — Rename apart**

Allocate a fresh `query_index` from the global counter for `nc`'s variables. All
`query_var` keys in `nc`'s `added_unifications` and effective body are remapped to use
this new `query_index`, making them distinct from every existing `query_var` in the
binding environment.

**Step 3 — Unify**

Using the base binding table at `nc`'s labels, unify `r.body[g]` with `nc`'s effective head. The result is a set of unification equations — the `added_unifications` for `n'`.

This **always succeeds**. The candidacy check found a unifying leaf in `nc`'s subtree. Every ancestor of that leaf (including `nc` itself, since `nc` is chosen from an uncovered, non-refuted part of the subtree) is strictly more general — fewer variable bindings — and therefore unifies at least as easily. There is no failure case to handle.

**Step 4 — Resolve**

Create a new child node `n'`:

```
n'.added_unifications = unification equations from Step 3
n'.added_body_goals   = terms from nc.effective_body that replace r.body[g]
n'.status             = pre_unfold
n'.children           = {}
```

Assign Euler-tour labels `open(n')` and `close(n')`. Solve `n'.added_unifications` in the combined context of the base binding table and the per-goal bind map at `r`'s labels (which contains the bindings accumulated during the candidacy traversal that led to `nc`), and record the concrete variable bindings in the base binding table at `open(n')` / `close(n')`. The per-goal bind map's bindings for the accepted candidate path are thus incorporated into the base bind map at `n'`'s position — this is how the candidate's binding environment is accepted when producing the offspring child node.

Insert `n'` into `r.children` keyed by `nc`'s rule ID. No edge object is created.

Since this is `r`'s first child, `r` transitions from leaf to internal. For every `(h, g_h)` in `r.status.candidate_existence_watchers`, refine `h`'s candidate set: remove the entry for `r` and add `n'` if `n'` passes the candidacy check for `h.body[g_h]`. Then clear `r.status.candidate_existence_watchers`.

`nc.effective_body` is obtained by path reconstruction on `nc`. The ancestor body goals of `r` are never copied — they remain implicit and are recovered by path reconstruction when needed.

The effective head of `n'` (recoverable via the base binding table at `n'`'s labels):

```
Effective head = unifier applied to r.effective_head
```

**Step 5 — Update wait set**

For every `mid_unfold` ancestor `s` of `nc` along the path from `nc`'s root to `nc`, add `s` to `r.status.wait_set`, establishing the dependency link `s → r`.

If `nc` itself is `mid_unfold` and is chosen directly (not a descendant of another `mid_unfold` node in the path), no wait set entry is created for `nc` — choosing an `mid_unfold` node directly covers its current state completely.

**Step 6 — Coverage constraint on `nc`**

`nc` is valid iff:

- No proper **ancestor** of `nc` (for this unfold context) has already been unfolded against — otherwise `nc`'s solutions are already subsumed
- No proper **descendant** of `nc` has already been unfolded against — otherwise descending would re-derive already-produced solutions

Coverage propagates upward: when **all children** of a node become covered, the node itself is transitively covered. This constraint is enforced by the caller supplying `nc`; the operation itself does not search for `nc`.

**Step 7 — Consume and refine**

Remove `nc`'s entry from `r.status.candidate_set` (or mark it covered).

If `nc` is internal (has children), the candidate slot for `nc` may be **refined**: replaced by references to the uncovered children of `nc` that still have unifying leaves. This is how entries evolve from root references toward finer-grained node references over multiple steps.

**Step 8 — Check state**

If `r.status.candidate_set` is now empty:
- If `r.status.wait_set` is non-empty: `r` remains `mid_unfold` in the starved condition
- If `r.status.wait_set` is empty: transition `r` to `post_unfold`; then propagate (see Step 9)

**Step 9 — Unfolded propagation**

When `r` transitions to `post_unfold`, for every node `h` that has `r` in its `wait_set`:

- Remove `r` from `h.status.wait_set`
- If `h.status.wait_set` is now empty and `h.status.candidate_set` is empty: transition `h` to `post_unfold` and repeat this step recursively for `h`

This propagation is part of the same atomic operation.

---

## 7. Size Invariant

Each unfold step adds **exactly one node** (Step 4), or zero (null propagation).

Total node count after `n` steps ≤ `|original axioms| + n`.

This is an identity, not just a bound. Remainders are never materialized as new rules — they live implicitly as the shrinking `candidate_set`. One candidate consumed = one new node = one step. The multiplicative feedback loop of the eager approach (k candidates → k new nodes → k² in the next round) is eliminated; growth is strictly additive.

---

## 8. Invariants

| Invariant | How it holds |
|-----------|-------------|
| **Completeness**: the global active cut covers all solutions | `mid_unfold` covers remaining `candidate_set` and pending `wait_set` obligations; children cover consumed candidates; together they partition the original candidate space |
| **Non-duplication**: no solution is derivable through two active-cut nodes | Consumed and remaining candidates are disjoint; coverage constraint prevents double-descending; wait-set obligations cover exactly the siblings not yet generated |
| **No self-unfolding**: `r` never unfolds over its own descendants | The treat-as-leaf rule prevents descendants from ever entering `r`'s candidate set |
| **Monotone specialization**: every node's head is a specialization of its root's head | Each resolved head applies the unifier to the parent's head; unifiers only add bindings |
| **One goal per rule**: body-goal committed once, never revised | `unfold_body_goal_idx` is set once at the `pre_unfold → mid_unfold` transition; there is no transition back to `pre_unfold` |

---

## 9. Special Cases

| Situation | Outcome |
|-----------|---------|
| `candidate_set` empty and `wait_set` empty at commit time | Null propagation: `r` immediately `post_unfold`; no child produced |
| `candidate_set` empty but `wait_set` non-empty at commit time | `r` enters `mid_unfold` in the starved condition; no child produced until a dependency fires |
| `|candidate_set| = 1` and `wait_set` empty at commit time | Unit propagation: single forced step; one child; `r` becomes `post_unfold` after |
| `nc` is chosen directly as an `mid_unfold` node | No wait set entry created; result covers `nc`'s current state completely; further specialization deferred to future unfolding of the child |
| `nc` is a descendant of one or more `mid_unfold` nodes | All `mid_unfold` ancestors of `nc` added to `r.status.wait_set`; `r` enters the starved condition once `candidate_set` empties |
| Dependency link fires: `s` produces new child `s_i` | `s_i` checked against `r`'s committed body-goal; admitted to `candidate_set` if it passes; `r` exits the starved condition |
| `s` becomes `post_unfold` | `s` removed from all wait sets; any node whose wait set thereby empties and whose candidate set is empty transitions to `post_unfold` |
| `nc` is `post_unfold` | Valid target; effective head and body intact via path reconstruction; unfolding proceeds normally |
| `nc` is a fact (`added_body_goals` empty) | Valid target; `n'.added_body_goals` is empty; the child's effective body has one fewer goal than `r`'s |
| All children of a node become covered | That node becomes transitively covered; its slot in any candidate set that referenced it is removed/refined |
| A node is refuted | Removed from the tree; all `candidate_existence_watchers` entries notified and the node's slot removed from those candidate sets; if any rule's candidate set thereby empties, it transitions to `post_unfold` or the starved condition accordingly; refutation propagates upward if all leaves in a parent's subtree are refuted |

---

## 10. Architectural Boundary

The iterative database is a **passive data structure**, not a reactive system. It does not loop, schedule, or call itself. All decisions (which rule to unfold, which body goal, which candidate) are supplied by an external caller.

**Interface:** the database exposes a single mutating operation:

```
unfold(subject_rule_id, body_goal_idx, candidate_rule_id) → stream<observation>
```

**Observations** streamed out as side-effects of the operation include any forced states that arise as a result of the step — specifically:

| Observation | Meaning |
|-------------|---------|
| `unit(rule_id, goal_idx)` | A `pre_unfold` rule now has exactly one candidate for `goal_idx` and an empty wait set — the next unfold at this goal is forced |
| `null(rule_id, goal_idx)` | A `pre_unfold` rule now has zero candidates for `goal_idx` and an empty wait set — this rule is forced to refute at this goal |

These conditions can arise at any point during the operation: at commit (Step 1), from leaf expansion (Step 4) updating other rules' candidate sets, from dependency links firing, or from refutation propagation. The database detects and reports them; it does not act on them.

The **external system** — not specified here — consumes the observation stream and decides when and whether to enact the forced unfolds by issuing further calls. This separation keeps the database's scope strictly bounded to state maintenance and change detection.
