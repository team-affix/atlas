# Progressive Database Unfolding — Specification

## 1. Database State

The database D is a **multitree**: a collection of nodes with no single global root. Every node was either loaded as an axiom (a root node) or produced by exactly one unfold step. There are no separate "tree" or "edge" objects — the database is simply the set of all nodes. Root nodes are those with no parent. Every non-root node's effective head is a **specialization** of its root ancestor's head (specialization = strictly additive instantiation; variable bindings are only added, never removed).

---

## 2. Structure: Nodes

There is one type: **node**.

| Field | Meaning |
|-------|---------|
| `status` | `pre_unfold`, `mid_unfold`, or `post_unfold` (see §3) |
| `added_bindings` | For a root node: `{var(0) → axiom_head}`; for a derived node: the unifier from the unification step that produced it |
| `added_body_goals` | For a root node: the axiom body; for a derived node: the unifier applied to `nc`'s effective body |
| `children` | A map from candidate rule ID to child node — each child is keyed by the ID of the candidate rule that was unfolded against to produce it |

`var(0)` is a reserved variable index serving as the universal head slot across the entire multitree. Every node's effective head is `var(0)` as resolved by composing `added_bindings` from the root ancestor down to that node. For a root node this immediately yields the axiom head; for derived nodes it yields the accumulated specialization along the path.

Root nodes are nodes with no parent. The database is the collection of root nodes; all descendants are reachable from them.

A node with no children is a **leaf** — an implicit property with no maintained set; a node is a leaf simply because `children` is empty.

### Status Variants

The `pre_unfold` status carries:

| Field | Meaning |
|-------|---------|
| `candidate_sets` | A vector of live candidate sets, one per body goal in order — each growing as new nodes pass the candidacy check for that goal |
| `candidate_existence_watchers` | The set of `(rule, goal_idx)` pairs whose candidate sets hold a reference to this node; preserved through the `pre_unfold → mid_unfold` transition; fired and cleared when the node gains its first child or is refuted |

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

Both the effective head and full body of any node `n` are recovered by walking the path from the root ancestor down to `n`.

**Effective head**: start with `var(0)`; at each node on the path, apply `node.added_bindings`. After applying the root node's `added_bindings`, `var(0)` resolves to the axiom head.

**Full body**: start with the root node's `added_body_goals` (the axiom body). At each subsequent node on the path, apply `node.added_bindings` to the current body, remove the goal at `parent.status.unfold_body_goal_idx`, and insert `node.added_body_goals` in its place.

`parent.status.unfold_body_goal_idx` is always available at each step since it is a field on both `mid_unfold` and `post_unfold` status variants.

This walk costs roughly the same as unification would, since each node typically binds at least one variable.

---

## 3. Node Status

| Status | Children | Meaning |
|--------|----------|---------|
| `pre_unfold` | 0 | Leaf; no unfolding decision made; `candidate_sets` holds a live set per body goal, each growing as the database grows |
| `mid_unfold` | ≥ 1 | Committed to `unfold_body_goal_idx`; children represent already-consumed candidates; `candidate_set` holds remaining ones |
| `post_unfold` | ≥ 0 | Candidate set and wait set exhausted; permanently done as a subject of unfolding |

**Starved** is a derived condition, not a stored state: a node is starved when `status == mid_unfold && candidate_set.empty() && !wait_set.empty()`. The `mid_unfold` status covers both the active-candidates case and the temporarily-starved case.

**Facts** (nodes with no body goals) stay `pre_unfold` permanently with an empty `candidate_sets` vector — there is no body-goal to commit `unfold_body_goal_idx` to. Whether a node is a fact is implicit from its `added_body_goals` being empty.

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

## 4. Candidate Sets

Every `pre_unfold` rule `r` maintains `r.status.candidate_sets`: a vector of live candidate sets indexed by body-goal position. **Initial content** is the set of root nodes (axioms) that pass the candidacy check for each body goal. In practice these sets are populated at load time and rarely change thereafter — the only general additions occur when entirely new axiom roots are introduced.

When `r` commits to unfolding at `g`, `r.status.candidate_sets[g]` becomes `r.status.candidate_set` in the new `mid_unfold` status (the other per-goal sets are discarded).

**Live additions via dependency links:** after committing, the primary source of new entries is the wait-set mechanism. When an unfold step for `r` descends below a `mid_unfold` node `s` to choose `nc`, a dependency link `s → r` is established (see §3 Wait Set and §6 Step 5). From that point forward, each new child `s_i` that `s` produces is offered to `r.status.candidate_set` — admitted if it passes the candidacy check, silently dropped otherwise. This is the main way candidate sets receive live updates at runtime: not from general database growth, but specifically from dependency links established when `r` descended into a not-yet-finished subtree to form a valid cut.

As entries are consumed, root references may be **refined** — replaced by references to specific descendant nodes to track candidacy at finer granularity (see §6, Step 7). Candidate entries are therefore node references in general, not necessarily root references.

**Leaf expansion propagation:** a candidate set entry is only valid while the referenced node is a leaf — leaves are the most-specialized points in a subtree and serve as the existence witnesses for candidacy. When a leaf node `n` gains its first child (because `n` itself becomes the subject of an unfold step), `n` is no longer a leaf and all candidate set entries pointing to it are stale. At that moment, every `(h, g_h)` in `n.candidate_existence_watchers` must have its `n` entry refined: `n` is removed and replaced by the new children of `n` that pass the candidacy check for `h.body[g_h]`. To support this, `pre_unfold` and `mid_unfold` both carry `candidate_existence_watchers` — the reverse index of which rules currently hold this node as a candidate entry. The set is populated when an entry is added to a candidate set. It is preserved through the `pre_unfold → mid_unfold` transition (Step 1 commits but creates no children yet, so the node is still a leaf) and fired and cleared in Step 4 when the first child appears. `post_unfold` nodes never gain children and therefore never need it.

Whether consumption is implemented as direct erasure from `candidate_set` or as a "covered" label on entries is an open implementation choice. The semantics are the same either way.

---

## 5. Candidacy

A node `n` is a **candidate for body-goal `g`** iff at least one leaf in `n`'s subtree unifies with `g`.

**Candidacy traversal** (top-down, depth-first, exploiting incremental unification):

- At node `n`: attempt to unify `g` with `n`'s effective head (only the newly instantiated parts need checking relative to the parent — incremental unification)
- If unification **fails**: prune the entire subtree. Every descendant is a stricter specialization, so descendants cannot unify if the ancestor does not
- If unification **succeeds** and `n` is a **leaf**: `n` is a candidate; stop immediately
- If unification **succeeds** and `n` is **internal**: recurse into children

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

Transition `r` to `mid_unfold(g, candidate_set, wait_set={})` where `candidate_set` is taken from `r.status.candidate_sets[g]` (the live set accumulated so far for that goal). The remaining per-goal sets are discarded. From this point, descendants of `r` are excluded from `candidate_set` by the treat-as-leaf rule; dependency-link additions may still arrive.

*Null propagation:* if `candidate_set` is empty and `wait_set` is empty, transition `r` to `post_unfold` immediately and produce no child.

*Starved at commit:* if `candidate_set` is empty but `wait_set` is non-empty, `r` enters the mid_unfold state in the starved condition — no child produced until a dependency fires.

*Unit propagation:* if `|candidate_set| = 1` and `wait_set` is empty, the following steps are forced.

**Step 2 — Rename apart**

Give `nc`'s variables a fresh frame offset disjoint from `r`'s variable range.

**Step 3 — Unify**

Compute `unifier` = MGU(`r.body[g]`, `nc`'s effective head).

This **always succeeds**. The candidacy check found a unifying leaf in `nc`'s subtree. Every ancestor of that leaf (including `nc` itself, since `nc` is chosen from an uncovered, non-refuted part of the subtree) is strictly more general — fewer variable bindings — and therefore unifies at least as easily. There is no failure case to handle.

**Step 4 — Resolve**

Create a new child node `n'`:

```
n'.added_bindings   = unifier
n'.added_body_goals = unifier applied to nc.effective_body
n'.status              = pre_unfold
n'.children            = {}
```

Insert `n'` into `r.children` keyed by `nc`'s rule ID. No edge object is created.

Since this is `r`'s first child, `r` transitions from leaf to internal. For every `(h, g_h)` in `r.status.candidate_existence_watchers`, refine `h`'s candidate set: remove the entry for `r` and add `n'` if `n'` passes the candidacy check for `h.body[g_h]`. Then clear `r.status.candidate_existence_watchers`.

`nc.effective_body` is obtained by path reconstruction on `nc`. The ancestor body goals of `r` are never copied — they remain implicit and are recovered by path reconstruction when needed.

The effective head of `n'` (recovered by composing `added_bindings` from root to `n'`):

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
