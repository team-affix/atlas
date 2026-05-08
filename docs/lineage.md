# Lineage

The lineage data structures collectively encode the derivation tree. Each individual lineage node represents a single step — either a goal or a resolution — and points back to its parent, forming a chain to the root. The full tree emerges from the set of all such chains.

## Node types

There are two interleaved node types. Each layer in the derivation alternates between them:

### `goal_lineage`

Represents a goal being worked on:
- `parent`: the `resolution_lineage` that produced this goal (which resolution's rule body contained it as a subgoal). `nullptr` for initial goals.
- `idx`: the **child index** within that parent resolution — which position in the rule body this goal occupies.

### `resolution_lineage`

Represents a decision to resolve a goal using a specific database rule:
- `parent`: the `goal_lineage` being resolved.
- `idx`: the **absolute index of the rule in the database**.

## Layers

Each layer in the derivation alternates node type: a `goal_lineage` is always resolved by a `resolution_lineage`, whose rule body produces further `goal_lineage` nodes, and so on.

Roots are the **initial goals** (`goal_lineage` nodes with `parent = nullptr`). A **leaf** `resolution_lineage` is one that has no child `goal_lineage` nodes in the current lineage tree — it is the outermost resolution on that branch.

## `lineage_pool`

Manages the lifetime of `goal_lineage` and `resolution_lineage` objects.

**Interning pool.** Lineages are interned by their two fields: `parent` pointer and `idx`. If you request a `goal_lineage{parent=X, idx=3}` and one already exists, the same object is returned. Each node's `parent` pointer links back toward the root, so following the chain from any node reconstructs the derivation path for that particular goal or resolution.

**Not backtracked.** Unlike `expr_pool`, the lineage pool does not roll back when a sim's trail frame is popped. This is essential because CDCL must reference lineages from *previous sims* — learned avoidances contain pointers into past sims' lineage trees, which must remain valid across restarts.

**Memory management via `pin()` / `trim()`:**
- `pin(rl)` — marks a resolution lineage (and its ancestors) as needed; called for each lineage in the decision lemma of the current sim.
- `trim()` — removes all lineages that were not pinned. Any derivation path not referenced by any learned avoidance is discarded.

The `bool` flag in the internal maps (`std::map<goal_lineage, bool>` etc.) tracks whether a lineage has been pinned.


