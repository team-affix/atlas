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
| `horizon` | Uses **CGW (Cumulative Grounded Weight)** as the MCTS reward. See [solvers.md](solvers.md). |
| `ridge` | Certainty-based. Focuses on finding and eliminating shallow conflicts. See [solvers.md](solvers.md). |
| `ridge-horizon` | *(planned)* Combines the strengths of both. |

Both use **CDCL**. See [cdcl.md](cdcl.md) and [solvers.md](solvers.md).

## Further reading

- [solvers.md](solvers.md) — Horizon vs Ridge reward functions
- [solving-loop.md](solving-loop.md) — The event-driven solving loop, unit/decided resolution, termination
- [resolution.md](resolution.md) — Resolution mechanics, variable copying, goal weights
- [cdcl.md](cdcl.md) — Trail, avoidances, lemmas
- [lineage.md](lineage.md) — Lineage structure, lineage_pool, expr_pool
- [events.md](events.md) — Event system, naming conventions, priority ordering
- [elimination.md](elimination.md) — active_eliminator, elimination_backlog
- [bootstrap.md](bootstrap.md) — Resolver, manifests, wiring
- [restart.md](restart.md) — Sim restart sequence
