# Progressive Database Unfolding — Specification

## 1. Database State

The database D is a **forest** F = {T₁, T₂, …} where each Tᵢ is a rooted **derivation tree**.

Each tree has a **root** that is an original (axiomatic) rule — never produced by unfolding. Every non-root node was produced by exactly one unfold step, and its effective head is a **specialization** of its root's head (specialization = strictly additive instantiation; variable bindings are only added, never removed).

**Open direction (§7):** the entire forest may ultimately be representable as one tree with a virtual root `X :- axiom(X)` whose children are the original axiom rules. If this works out, forest-level bookkeeping collapses into the single tree structure. Left open for now.

---

## 2. Node Representation

**Root nodes** carry a full rule: `head`, `body`, `var_count`.

**Non-root nodes** carry a **delta** only — no full head copy:

| Field | Meaning |
|-------|---------|
| `n.bindings` | The substitution θ produced when the parent's body-goal `g` was unified with the candidate node's head |
| `n.introduced_body` | The candidate node's body goals, after applying θ (the goals that replaced `parent.body[g]`) |

The full head of `n` is recovered by composing `bindings` along the path from the root down to `n`. The full body of `n` is computed similarly by walking the path, applying each θ, and splicing in each `introduced_body` at the appropriate position. This walk costs roughly the same as unification would, since each step typically binds at least one variable.

---

## 3. Node Status

| Status | Children | Additional state | Meaning |
|--------|----------|-----------------|---------|
| `fresh` | 0 | — | Leaf; no unfolding decision made; all body-goal candidate sets are live and receiving updates |
| `suspended` | ≥ 1 | `goal_idx` | Unfolding in progress at body-goal `goal_idx`; that goal's candidate set is severed; children represent already-consumed candidates; the severed set IS the remaining set |
| `exhausted` | ≥ 1 | — | All candidates consumed; node is fully covered by its children |

`suspended` needs only `goal_idx` as additional state. There is no separate `remaining` field — see §5.

---

## 4. Candidate Sets

Every body-goal `g` of every rule `r` has an associated **live candidate set** `C(r, g)`.

**While `r` is fresh:** `C(r, g)` is kept up-to-date continuously. Whenever a new node `n` appears anywhere in the database and passes the candidacy check for `r.body[g]`, a reference to `n`'s subtree is added to `C(r, g)`.

**At the moment `r` begins unfolding at `g`:** `C(r, g)` is **severed** — it stops receiving additions from the rest of the database. The severed set is the starting remaining set with no copy or translation needed. Subsequent unfold steps consume entries from this same set.

**Initial content of `C(r, g)`:** entries are references to **tree roots**. As the system runs and trees grow, entries may be **refined** — a root reference replaced by references to specific subtree nodes within that tree, as candidates are consumed and the tracking granularity increases (see §6, step 7). Candidate entries are therefore **subtree node references** in general, not necessarily root references.

Whether consumption is implemented as direct erasure from `C(r, g)` or as a "covered" label on entries is an open implementation choice. The semantics are the same either way.

---

## 5. Candidacy

Tree T is a **candidate for body-goal `g`** iff at least one leaf of T unifies with `g`.

**Candidacy traversal** (top-down, depth-first, exploiting incremental unification):

- At node `n`: attempt to unify `g` with `n`'s effective head (only the newly instantiated parts need checking relative to the parent — incremental unification)
- If unification **fails**: prune the entire subtree. Every descendant is a stricter specialization, so descendants cannot unify if the ancestor does not
- If unification **succeeds** and `n` is a **leaf**: T is a candidate; stop immediately
- If unification **succeeds** and `n` is **internal**: recurse into children

This is an **existence check**, not enumeration. The traversal halts at the first unifying leaf. In the worst case the entire tree is traversed to disprove candidacy; early subtree pruning is the primary source of speed-up from unfolding.

**The candidate unit is the whole tree** (or subtree entry in `C`), not the particular leaf found.

---

## 6. The Atomic Unfold Operation

The operation takes as input:

- `r`: the rule to unfold (a `fresh` or `suspended` node in the forest)
- `g`: the body-goal index (required if `r` is fresh; must match `r.goal_idx` if suspended)
- `nc`: the candidate node to unfold against (a node within some entry in `C(r, g)`)

and produces at most one new node.

**Step 1 — Sever (only if `r` is fresh)**

Transition `r` to `suspended(g)`. From this point `C(r, g)` stops receiving additions. No active computation: the set was already being maintained.

*Null propagation:* if `C(r, g)` is empty at this point, transition `r` to `exhausted` immediately and produce no child. `r` contributed no solutions; its removal from the active cut is sound.

*Unit propagation:* if `|C(r, g)| = 1`, this and the following steps are forced.

**Step 2 — Rename apart**

Give `nc`'s variables a fresh frame offset disjoint from `r`'s variable range.

**Step 3 — Unify**

Compute θ = MGU(`r.body[g]`, `nc`'s effective head).

This **always succeeds**. Reason: the candidacy check for `C(r, g)` found a unifying leaf in `nc`'s tree. Every ancestor of that leaf is strictly more general (fewer variable bindings), and thus unifies at least as easily. The coverage constraint (Step 5 below) guarantees `nc` is an ancestor of an as-yet-uncovered unifying leaf. Therefore θ exists. There is no failure case to handle.

**Step 4 — Resolve**

Form the new rule `r'` as a delta:

```
r'.bindings         = θ  (on r's head)
r'.introduced_body  = θ( nc.effective_body )

Full head  = θ( r.effective_head )
Full body  = θ( r.body[0..g-1] )  ++  θ( nc.effective_body )  ++  θ( r.body[g+1..] )
```

**Step 5 — Coverage constraint on `nc`**

`nc` is valid iff:

- No proper **ancestor** of `nc` (within its tree, for this `(r, g)` context) has already been unfolded against — otherwise `nc`'s solutions are already subsumed
- No proper **descendant** of `nc` has already been unfolded against — otherwise descending would re-derive already-produced solutions

Coverage propagates upward: when **all children** of a node become covered, the node itself is transitively covered and is no longer a valid `nc` candidate.

This constraint is enforced by the caller supplying `nc`. The operation itself does not search for `nc`.

**Step 6 — Insert child**

Add `r'` as a child of `r` in `r`'s tree with status `fresh`.

**Step 7 — Consume and refine**

Remove `nc`'s entry from `C(r, g)` (or mark it covered).

If `nc` was not a leaf (i.e., nc has children in its own tree), the candidate slot for `nc` may be **refined**: replaced by references to the uncovered children or siblings of `nc` that still have unifying leaves. This is how entries evolve from root references toward finer subtree references over multiple steps.

**Step 8 — Check exhaustion**

If `C(r, g)` is now empty, transition `r` to `exhausted`. `r` leaves the active cut; its children's subtrees cover it completely.

---

## 7. Size Invariant

Each unfold step adds **exactly one node** (Step 6), or zero nodes (null propagation).

Total node count after `n` steps ≤ `|original rules| + n`.

This is an identity, not just a bound. Remainders are never materialized as new rules — they live implicitly as the shrinking `C(r, g)`. One candidate consumed = one new node = one step. The multiplicative feedback loop of the eager approach (k candidates → k new nodes → k² in the next round) is eliminated; growth is strictly additive.

---

## 8. Invariants

| Invariant | How it holds |
|-----------|-------------|
| **Completeness**: the global active cut covers all solutions | `suspended(g)` covers remaining `C(r,g)`; children cover consumed candidates; they partition the original frozen set |
| **Non-duplication**: no solution is derivable through two active-cut nodes | Consumed and remaining candidates are disjoint; coverage constraint prevents double-descending |
| **No self-unfolding**: `r` never unfolds over its own descendants | `C(r, g)` is severed before `r`'s first child exists; post-sever additions are rejected |
| **Monotone specialization**: every node's head is a specialization of its root's head | Each resolved head = θ(parent head); θ only adds bindings |
| **One goal per rule**: body-goal committed once, never revised | `suspended` carries a fixed `goal_idx`; there is no transition back to `fresh` |

---

## 9. Special Cases

| Situation | Outcome |
|-----------|---------|
| `C(r, g)` empty at sever time | Null propagation: `r` immediately exhausted; no child produced; `r` exits cut |
| `|C(r, g)| = 1` at sever time | Unit propagation: single forced step; one child; `r` exhausted after |
| `nc` is the root of its tree (shallow unfold) | Valid; result less specialized but complete; further unfolding of `r'` later can exploit any depth the tree has grown to by then |
| `nc` is a deep leaf of its tree | Maximum specialization in one step; `nc`'s siblings may still be referenced in `C(r, g)` |
| Tree Tc grows after its root entered `C(r, g)` | New descendants are free; they can be chosen as `nc` on any future step that consumes Tc's slot |
| All children of a node become covered | That node becomes transitively covered; its slot in any `C` that referenced it is removed/refined |
