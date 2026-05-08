# Resolution

## What happens during a resolution

When a goal is resolved against a candidate clause:

1. **Unification:** The goal's expression is unified with the head of the candidate clause. Variables are bound in the `bind_map`.
2. **Subgoal expansion:** A fresh copy of the candidate's rule body is produced (variables renamed via `translation_map`). Each atom becomes a new child goal.
3. **Goal store update:**
   - The resolved goal is **deactivated**: removed from all goal stores, inserted into `inactive_goal_store`.
   - All child goals are **activated**: inserted into the active goal stores.

If the candidate is a fact (empty body), no child goals are produced and the resolved goal simply moves to inactive. If the active goal store then becomes empty, `solved_event` is emitted.

## Variable copying (translation_map)

Rule variables must be freshened to avoid clashing with variables already in use:

1. For every variable in the rule head, a fresh variable ID is allocated (via `var_sequencer`) and recorded in the `translation_map`.
2. The head expression is unified with the goal expression using this mapping, producing bindings in the `bind_map`.
3. The rule body expressions are copied using the same `translation_map`. Variables that appear only in the body (not the head) get new entries in the map on the fly.
4. The resulting copied body expressions become the expressions of the new child goals.

## `expr_pool`

A **backtrackable interning pool** for expressions. All expressions in the system are allocated through it.

**Interning:** Structurally identical expressions are stored only once, reducing memory usage.

**Important:** Interning is purely a memory optimisation. The solver does **not** rely on pointer identity for equality — expressions are compared structurally, not by address.

**Backtrackability:** Expressions allocated during a sim are automatically reclaimed when the sim's trail frame is popped, preventing unbounded growth across restarts.

**Immutability:** Expressions are immutable once created. Variable bindings are tracked separately in the `bind_map`, not by mutating expression nodes.

## Variable bindings and `bind_map`

The `bind_map` is a **global substitution map** scoped to the current sim. All active goals share it.

When the sim ends, all bindings are undone via the trail — popping the sim's trail frame atomically restores the map to its state at the start of the sim. Variable bindings do **not** persist across sims.

## Goal weights and CGW

Each goal carries a **weight**, tracked in the `goal_weight_store`. Used by the `horizon` solver as the MCTS reward signal.

**Initialisation:** Initial goals are assigned weights that sum to exactly `1.0` (normalised).

**Propagation:** When a goal with weight `w` is resolved against a rule with `n` body atoms, each child goal receives weight `w / n`. If the rule is a fact (`n = 0`), the weight is "cashed in" — it contributes to the **CGW (Cumulative Grounded Weight)** reward for the sim.

**Invariant:** The sum of all active goal weights plus all already-grounded weight always equals `1.0` (up to floating-point error). CGW is therefore a probability-like measure of how much of the proof tree has been successfully grounded so far.

## Goal initialisation vs. goal expansion

When a goal becomes active, three things must be populated: its **expression**, **weight**, and **candidate set**.

### Initial goals

Populated by initialiser entities during the `initial_goal_activating` phase:

| Entity | Store | What it sets |
|---|---|---|
| `initial_goal_expr_initializer` | `goal_expr_store` | Expression for the initial goal (from the command-line goal). |
| `initial_goal_weight_initializer` | `goal_weight_store` | Weight, normalised so all initial weights sum to `1.0`. |
| `initial_goal_candidates_initializer` | `goal_candidates_store` | Full database as initial candidate set. |

### Derived goals

Populated by expander entities in response to `goal_activating_event`:

| Entity | Store | What it sets |
|---|---|---|
| `goal_expr_expander` | `goal_expr_store` | Copied body expression (using `translation_map` from parent resolution). |
| `goal_weight_expander` | `goal_weight_store` | Parent weight divided by number of siblings: `w / n`. |
| `goal_candidates_expander` | `goal_candidates_store` | Full database as candidate set. |

### Candidate set initialisation

Currently, both initial and derived goals receive the **entire database** as their starting candidate set. A future optimisation would filter to only rules whose head functor matches the goal's functor.

Once candidates are inserted, `goal_activated_event` bridges to `goal_candidates_changed_event`, feeding the detectors and flushing the elimination backlog.

## `normalizer`

Not used internally by the core solving loop. Exists as a utility for external consumers (e.g. the CLI) that need to present variable bindings back to the user in a readable form.
