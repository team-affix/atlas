# Progressive Database Unfolding — Specification

## 1. Database State

The database D is a **multitree**: a collection of rooted derivation trees sharing no nodes, with no single global root. Each tree has its own root (an axiom rule) and grows downward as unfolding proceeds.

There are no separate "tree" objects. The database is simply the set of all nodes and the edges between them. Root nodes are those with no incoming edge (null parent). Every other node was produced by exactly one unfold step, and its effective head is a **specialization** of its root's head (specialization = strictly additive instantiation; variable bindings are only added, never removed).

---

## 2. Structure: Roots, Nodes, and Edges

The multitree is built from three types: **root**, **node**, and **edge**.

### Root

A root is the entry point for one axiom tree in the multitree:

| Field | Meaning |
|-------|---------|
| `head` | The head literal of the original axiom rule |
| `node` | The root node for this tree |

The multitree is a collection of roots. The head lives here, not on the node, so all nodes are uniform.

### Node

A node carries:

| Field | Meaning |
|-------|---------|
| `body` | The full accumulated body of the rule at this node — for a root node, the axiom's original body; for a non-root node, the fully resolved body computed at creation time |
| `status` | `fresh`, `suspended(goal_idx)`, `starved(goal_idx)`, or `exhausted` (see §3) |
| `outgoing_edges` | The list of edges to children |
| `wait_set` | The set of suspended rules this node is waiting on (see §3); may be non-empty in both `suspended` and `starved` states |

All nodes are the same type. There are no special root-node fields on the node itself.

A node with no outgoing edges is a **leaf**. This is an implicit property — there is no maintained leaf set. A node is a leaf simply because `outgoing_edges` is empty.

### Edge

An edge carries the transformation from a parent node to its child node:

| Field | Meaning |
|-------|---------|
| `bindings` | The substitution θ from unifying `parent.body[goal_idx]` with `nc`'s effective head |
| `child` | The child node contained within this edge |

`goal_idx` is not carried on the edge. All outgoing edges from a given parent share the same `goal_idx` (the body-goal is committed once and never changes), so the edge is the wrong place for it. It lives in the parent's `suspended` or `starved` status for as long as it is needed for unfolding decisions.

An edge owns and contains its child node. A node's children are reached through its outgoing edges, receiving both the transformation data and the child node as one unit.

### Path Reconstruction

The **effective head** of any node `n` is recovered by walking the path of edges from the root down to `n` and composing bindings:

- Start with `root.head`; at each edge on the path, apply `edge.bindings`

This walk costs roughly the same as unification would, since each edge typically binds at least one variable.

The **full body** of any node `n` is stored directly on `n` — it is materialized at the time the node is created (see §6 Step 4). No path walk is needed to recover it. The head uses the delta/compose approach; the body does not.

---

## 3. Node Status

| Status | Outgoing edges | Additional state | Meaning |
|--------|---------------|-----------------|---------|
| `fresh` | 0 | — | Leaf; no unfolding decision made; all body-goal candidate sets are live and receiving updates |
| `suspended` | ≥ 1 | `goal_idx` | Unfolding in progress; candidate set `C(r, goal_idx)` is severed and non-empty; outgoing edges represent already-consumed candidates; wait set may also be non-empty |
| `starved` | ≥ 0 | `goal_idx` | Candidate set is empty but wait set is non-empty; no candidates to consume right now; transitions back to `suspended` when a dependency link delivers a new candidate |
| `exhausted` | ≥ 0 | — | Candidate set empty and wait set empty; permanently done as a subject of unfolding |

`starved` is the conjunction of: no candidates currently available AND at least one wait set entry. A `suspended` rule can have a non-empty wait set simultaneously with a non-empty candidate set — it is only `starved` once the candidate set empties while wait entries remain.

**Facts** (rules with no body goals) are `exhausted` immediately on load — there is no body-goal to commit to and no candidates can ever exist. They are permanent leaves and valid targets.

**Critical distinction — subject vs target:**

- **Subject of unfolding** (the rule `r` being unfolded): must be `fresh` or `suspended`; these form the active cut
- **Target of unfolding** (the `nc` another rule resolves against): any node — `fresh`, `suspended`, `starved`, or `exhausted`

Starved and exhausted nodes leave the active cut only as subjects. They remain fully reachable as targets.

### Wait Set

The `wait_set` of a rule `r` (present in both `suspended` and `starved` states) is a set of `suspended` rules. Each entry `s ∈ wait_set(r)` means:

- `r` descended below `s` when choosing `nc`, and therefore committed to covering all of `s`'s future children as part of `r`'s own cut
- Whenever `s` produces a new child `s_i`, `s_i` is offered to `C(r, g)` subject to a candidacy check against `r.body[g]`; if it passes, it is added to `C(r, g)` and `r` transitions from `starved` to `suspended` (if `r` was `starved`)
- When `s` becomes `exhausted`, `s` is removed from `wait_set(r)`; if `wait_set(r)` is then empty and `C(r, g)` is still empty, `r` transitions from `starved` to `exhausted`

**Which ancestors enter the wait set:** when `r` unfolds against `nc` and `nc` is a descendant of a suspended node `s` in `nc`'s tree, every suspended ancestor of `nc` along the path from `nc`'s tree root to `nc` is added to `wait_set(r)`. If `nc` is chosen directly as a suspended node (not a descendant of it), no wait set entry is created — the result covers `nc`'s current state completely and further specialization is deferred to future unfolding of the resulting child.

---

## 4. Candidate Sets

Every body-goal `g` of every rule `r` has an associated **live candidate set** `C(r, g)`.

**While `r` is fresh:** `C(r, g)` is kept up-to-date continuously. Whenever a new node `n` appears anywhere in the database and passes the candidacy check for `r.body[g]`, a reference to `n` is added to `C(r, g)`.

**At the moment `r` begins unfolding at `g`:** `C(r, g)` is **severed** — it stops receiving additions from the rest of the database. The severed set is the starting remaining set with no copy or translation needed. Subsequent unfold steps consume entries from this same set.

**Dependency-link additions:** after severing, `C(r, g)` can still receive entries from dependency links firing (see §3 Wait Set). Each such candidate is checked against `r.body[g]` before being admitted — a new sibling `s_i` from a suspended `s` is a further specialization of `s`'s head and may fail to unify; if so it is silently discarded.

**Initial content of `C(r, g)`:** entries are references to **root nodes** (axioms). As the system runs and trees grow, entries may be **refined** — a root reference replaced by references to specific descendant nodes, as candidates are consumed and tracking granularity increases (see §6, step 6). Candidate entries are therefore **node references** in general, not necessarily root references.

Whether consumption is implemented as direct erasure from `C(r, g)` or as a "covered" label on entries is an open implementation choice. The semantics are the same either way.

---

## 5. Candidacy

A node `n` is a **candidate for body-goal `g`** iff at least one leaf in `n`'s subtree unifies with `g`.

**Candidacy traversal** (top-down, depth-first, exploiting incremental unification):

- At node `n`: attempt to unify `g` with `n`'s effective head (only the newly instantiated parts need checking relative to the parent — incremental unification)
- If unification **fails**: prune the entire subtree. Every descendant is a stricter specialization, so descendants cannot unify if the ancestor does not
- If unification **succeeds** and `n` is a **leaf**: `n` is a candidate; stop immediately
- If unification **succeeds** and `n` is **internal**: recurse into children

**Self-candidacy and the treat-as-leaf rule:** when building `C(r, g)` and the traversal reaches `r` itself, `r` is treated as a leaf regardless of its actual status — the traversal does not recurse into `r`'s children. If `r`'s head unifies with `r.body[g]`, `r` is added as a candidate for itself; if not, `r`'s subtree is pruned. Either way, `r`'s proper descendants are never reachable by the traversal and therefore never enter `C(r, g)`. This enforces the no-self-unfolding invariant structurally.

This is an **existence check**, not enumeration. The traversal halts at the first unifying leaf. In the worst case the entire subtree is traversed to disprove candidacy; early subtree pruning is the primary source of speed-up from unfolding.

---

## 6. The Atomic Unfold Operation

The operation takes as input:

- `r`: the rule to unfold (a `fresh` or `suspended` node)
- `g`: the body-goal index (required if `r` is fresh; must match `r.goal_idx` if suspended)
- `nc`: the candidate node to unfold against (any node — `fresh`, `suspended`, `stuck`, or `exhausted` — referenced in `C(r, g)`)

and produces at most one new edge and node.

**Step 1 — Sever (only if `r` is fresh)**

Transition `r` to `suspended(g)`. From this point `C(r, g)` stops receiving additions from the database (dependency-link additions may still arrive — see §4). No active computation: the set was already being maintained.

*Null propagation:* if `C(r, g)` is empty at this point and `wait_set(r)` is empty, transition `r` to `exhausted` immediately and produce no child. If `C(r, g)` is empty but `wait_set(r)` is non-empty, transition to `starved`.

*Unit propagation:* if `|C(r, g)| = 1` and `wait_set(r)` is empty, this and the following steps are forced.

**Step 2 — Rename apart**

Give `nc`'s variables a fresh frame offset disjoint from `r`'s variable range.

**Step 3 — Unify**

Compute θ = MGU(`r.body[g]`, `nc`'s effective head).

This **always succeeds**. The candidacy check found a unifying leaf in `nc`'s subtree. Every ancestor of that leaf (including `nc` itself, since `nc` is chosen from an uncovered, non-refuted part of the subtree) is strictly more general — fewer variable bindings — and therefore unifies at least as easily. There is no failure case to handle.

**Step 4 — Resolve**

Create a new edge `e'` and child node `n'`:

```
e'.bindings  = θ

n'.body      = θ( r.body[0..g-1] )  ++  θ( nc.body )  ++  θ( r.body[g+1..] )
n'.status    = fresh

e'.child     = n'
```

Append `e'` to `r.outgoing_edges`.

`g` comes from `r`'s `suspended` status. `r.body` and `nc.body` are read directly from the nodes (fully materialized). The full body is computed once and stored on `n'`; no reconstruction walk is needed later.

The effective head of `n'` (recovered by composing bindings from root to `n'`):

```
Effective head  = θ( r.effective_head )
```

**Step 5 — Update wait set**

For every suspended ancestor `s` of `nc` along the path from `nc`'s tree root to `nc`:

- Add `s` to `wait_set(r)` (establishing the dependency link `s → r`)

If `nc` itself is `suspended` and is chosen directly (i.e. `nc` has no suspended ancestors between it and the candidate root that `r` already descended through), no wait set entry is created for `nc` itself — choosing a suspended node directly covers its current state completely.

**Step 6 — Coverage constraint on `nc`**

`nc` is valid iff:

- No proper **ancestor** of `nc` (for this `(r, g)` context) has already been unfolded against — otherwise `nc`'s solutions are already subsumed
- No proper **descendant** of `nc` has already been unfolded against — otherwise descending would re-derive already-produced solutions

Coverage propagates upward: when **all children** of a node become covered, the node itself is transitively covered. This constraint is enforced by the caller supplying `nc`; the operation itself does not search for `nc`.

**Step 7 — Consume and refine**

Remove `nc`'s entry from `C(r, g)` (or mark it covered).

If `nc` is internal (has children), the candidate slot for `nc` may be **refined**: replaced by references to the uncovered children of `nc` that still have unifying leaves. This is how entries evolve from root references toward finer-grained node references over multiple steps.

**Step 8 — Check suspension state**

If `C(r, g)` is now empty:
- If `wait_set(r)` is non-empty: transition `r` to `starved(g)`
- If `wait_set(r)` is empty: transition `r` to `exhausted`; then propagate (see Step 9)

**Step 9 — Exhaustion propagation**

When `r` transitions to `exhausted`, for every rule `h` that has `r` in its `wait_set`:

- Remove `r` from `wait_set(h)`
- If `wait_set(h)` is now empty and `C(h, goal_idx(h))` is empty: transition `h` from `starved` to `exhausted` and repeat this step recursively for `h`

This propagation is part of the same atomic operation.

---

## 7. Size Invariant

Each unfold step adds **exactly one edge and one node** (Step 4), or zero (null propagation).

Total node count after `n` steps ≤ `|original axioms| + n`.

This is an identity, not just a bound. Remainders are never materialized as new rules — they live implicitly as the shrinking `C(r, g)`. One candidate consumed = one new node = one step. The multiplicative feedback loop of the eager approach (k candidates → k new nodes → k² in the next round) is eliminated; growth is strictly additive.

---

## 8. Invariants

| Invariant | How it holds |
|-----------|-------------|
| **Completeness**: the global active cut covers all solutions | `suspended(g)` covers remaining `C(r,g)`; `starved` covers pending wait-set obligations; children cover consumed candidates; together they partition the original candidate space |
| **Non-duplication**: no solution is derivable through two active-cut nodes | Consumed and remaining candidates are disjoint; coverage constraint prevents double-descending; wait-set obligations cover exactly the siblings not yet generated |
| **No self-unfolding**: `r` never unfolds over its own descendants | `C(r, g)` is severed before `r`'s first child exists; the treat-as-leaf rule prevents descendants from ever entering `C(r, g)` |
| **Monotone specialization**: every node's head is a specialization of its root's head | Each resolved head = θ(parent head); θ only adds bindings |
| **One goal per rule**: body-goal committed once, never revised | `suspended` and `starved` both carry a fixed `goal_idx`; there is no transition back to `fresh` |

---

## 9. Special Cases

| Situation | Outcome |
|-----------|---------|
| `C(r, g)` empty and wait set empty at sever time | Null propagation: `r` immediately exhausted; no child produced |
| `C(r, g)` empty but wait set non-empty at sever time | `r` immediately starved; no child produced until a dependency fires |
| `|C(r, g)| = 1` and wait set empty at sever time | Unit propagation: single forced step; one child; `r` exhausted after |
| `nc` is chosen directly as a suspended node | No wait set entry created; result covers `nc`'s current state completely; further specialization deferred to future unfolding of the child |
| `nc` is a descendant of one or more suspended nodes | All suspended ancestors of `nc` added to `wait_set(r)`; `r` becomes `starved` once `C(r,g)` empties |
| Dependency link fires: `s` produces new child `s_i` | `s_i` checked against `r.body[g]`; admitted to `C(r, g)` if it passes; `r` transitions from `starved` to `suspended` |
| `s` becomes exhausted | `s` removed from all wait sets; any rule whose wait set thereby empties and whose candidate set is empty transitions from `starved` to `exhausted` |
| `nc` is exhausted | Valid target; effective head and body intact; unfolding proceeds normally |
| `nc` is a fact (exhausted, no body goals) | Valid target; `n'.body` is empty; the resolved child has one fewer body goal |
| All children of a node become covered | That node becomes transitively covered; its slot in any `C` that referenced it is removed/refined |
