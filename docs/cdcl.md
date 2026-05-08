# CDCL and the Trail

## Trail frames

The trail supports a stack of **frames** (push/pop). Atlas uses two distinct levels:

**Base frame (persistent):** Learning always happens here. Lemmas derived from conflicts are added at the base level and survive across restarts. This ensures the same conflicting decision set is never revisited.

**Sim frame (ephemeral):** At the start of each sim, a new frame is pushed. All resolution activity — variable bindings, goal activations, candidate eliminations — happens within this frame. When the sim ends, the frame is popped, undoing everything.

## Avoidances

An **avoidance** is a **NAND constraint** over a set of resolution lineages. It asserts that the solver must never simultaneously commit to *all* of the listed decisions. It is the Horn-clause analogue of a learned clause in SAT-CDCL.

`constrain(rl)` is called for every resolution made — unit or decided. CDCL looks up which avoidances contain `rl` and removes it — narrowing the constraint. An avoidance becomes:

- **Unit** — exactly one lineage remains → the solver is forced to **eliminate** that remaining candidate, since choosing it would complete the avoided set and cause a conflict.
- **Empty** — no lineages remain → conflict.

## Lemmas

A **lemma** is a **leaf-only set of resolution lineages**. It is derived by discarding all non-leaf entries from a set of resolution lineages. See [lineage.md](lineage.md) for the definition of a leaf `resolution_lineage`.

A leaf lineage *implies* all its ancestors. Keeping only leaves is lossless: no information is lost, just redundancy removed.

### Why leaf-only in avoidances

If intermediary lineages were stored in avoidances, `constrain()` would generate hits every time an intermediate resolution was made. But what we care about is whether the outermost (leaf) resolutions in the lineage tree have been taken — those are the ones whose presence or absence determines whether an avoidance is satisfied. Tracking only leaves gives clean, non-spurious unit/empty detection.

### Decision lemmas

Used to prevent revisiting a conflicting decision set. Given to CDCL via `learn()`, which creates a new avoidance from it.

### Resolution lemmas (proofs)

Used to represent a completed proof compactly. The leaf set is a minimal sufficient description of the full proof tree. These are what the solver returns as witnesses when it finds a solution.

## On conflict

1. A decision lemma is derived from the decisions made during the sim.
2. The sim frame is popped — all bindings and goal state are undone.
3. The lemma is learned at the base frame level.
4. A new sim begins with the lemma already applied — CDCL constrains candidates to avoid the conflicting path.
