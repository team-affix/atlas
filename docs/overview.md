# Atlas

Atlas is a CHC (Constrained Horn Clause) solver.

## Input

Atlas reads Horn clause databases from `.chc` files. The format is closely modelled on Prolog — essentially a standard Horn clause database.

**Syntax:**
- Facts: `parent(tom, bob).`
- Rules: `ancestor(X, Z) :- parent(X, Y), ancestor(Y, Z).`
- Variables are uppercase; atoms, functors, and numbers are lowercase.
- Standard Prolog list syntax is supported: `[H|T]`, `[]`, `[a, b, c]`.
- Line comments use `%`.

**Constraints:** The clause language is purely relational (uninterpreted). There are no built-in arithmetic or bitvector constraints — arithmetic is encoded explicitly using Peano-style or functor-based representations.

**Goals:** Initial goals are specified on the command line, not inside the file. Goals may be conjunctive (comma-separated). The solver to use is also specified on the command line. Example:

```
atlas horizon arithmetic/db.chc --goal "even(X), lt(X, suc(suc(suc(suc(suc(suc(zero)))))))"
```

**Example domains covered by the bundled examples:** family trees, Peano arithmetic, list membership, graph reachability, binary trees, Boolean SAT, and expression synthesis.

## Solvers

Atlas exposes multiple solvers, each with different search strategies:

| Solver | Description |
|---|---|
| `horizon` | Heuristic-guided search using **CGW (Cumulative Grounded Weight)** reward — a measure of how many goals have been resolved against facts. Prioritises paths that feel like they are making progress toward grounding. |
| `ridge` | Certainty-based search. Has no notion of "closeness to a solution." Instead, it rules out what provably cannot work and then treats all remaining candidate paths fairly. Its heuristic focuses on finding and eliminating **shallow conflicts** first. |
| `ridge-horizon` | *(planned)* Combines the strengths of both: certainty-based pruning from `ridge` with the progress-aware heuristic of `horizon`. |

Both solvers use **CDCL (Conflict-Driven Clause Learning)** to avoid revisiting the same decision set twice.

## Core Solving Loop

The solver maintains a set of **active goals** — goals that are currently being worked on. Each goal has a set of **candidates**: entries in the clause database that could potentially be used to resolve it.

Each iteration of the loop proceeds as follows, in priority order:

1. **Unit propagation:** If any active goal has exactly one remaining candidate, resolve it immediately — no decision needed.
2. **Conflict detection:** If any active goal has *zero* remaining candidates, emit a conflict. CDCL kicks in to learn a lemma and backtrack.
3. **Solved detection:** If the active goal store is empty, emit a solved event — a solution has been found.
4. **Decision:** If no unit goals exist, use **MCTS (Monte Carlo Tree Search)** to decide:
   - First, which active goal to resolve next.
   - Then, which candidate (database clause) to resolve it against.

   MCTS operates in two layers, one for goal selection and one for candidate selection.

The solver is **complete with respect to CDCL**: it will never revisit the same combination of decisions twice, bounding the search space even for infinite clause databases.

## Resolution

When a goal is resolved against a candidate clause:

1. **Unification:** The goal's expression is unified with the head of the candidate clause. Variables are bound accordingly.
2. **Subgoal expansion:** A fresh copy of the candidate's rule body is produced (with variables renamed to avoid collisions). Each atom in the body becomes a new **child goal**, carrying the variable bindings inherited from the unification.
3. **Goal store update:**
   - The resolved goal is **deactivated**: removed from all goal stores and inserted into the `inactive_goal_store`.
   - All child goals are **activated**: inserted into the active goal stores and immediately considered for the next iteration of the loop.

If the candidate was a fact (empty body), no child goals are produced and the resolved goal simply moves to inactive. If the active goal store then becomes empty, a solution is emitted.

---

*This document is a work in progress, built through guided documentation sessions.*
