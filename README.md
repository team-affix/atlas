# Atlas

Atlas is a **pure relational CHC (Constrained Horn Clause) solver** for goal-directed Horn logic. A **database** of Horn rules defines the problem structure; the **query** (conjunctive goal with free variables) is the problem to solve. Atlas searches for satisfying substitutions and enumerates distinct models until the decision space is exhausted.

**Completeness (design contract):**

| Property | Guarantee |
|---|---|
| Model-complete | If a satisfying substitution exists, Atlas finds it (all domains). |
| Refutationally complete | If no solution exists and the search space is **finite**, Atlas eventually reports `REFUTED`. |
| Refutation (general) | Over **infinite** domains, refutation is not guaranteed; Atlas may still terminate with `REFUTED` on some problems. |
| Sound | Every reported `SOLVED` binding is a valid model of the goal under the database. |

Formal proofs of these properties are in progress.

---

## Quick start

```bash
make atlas
./build/atlas horizon cli/examples/ancestor/db.chc --goal "ancestor(tom, X)"
```

```
SOLVED
X = bob
SOLVED
X = liz
...
REFUTED
```

Solver subcommands: `horizon`, `ridge`, `basic`. The query is passed with `--goal` as comma-separated atoms.

---

## What Atlas is for

Horn clauses are a universal encoding for search: reachability, enumeration, constraint satisfaction, synthesis, and combinatorial exploration. Each instance pairs a **rule database** (the structure of the domain) with a **query** (what to find). Atlas targets **Turing-complete search problems** expressed in this form.

Unlike theory-heavy CHC solvers (e.g. Spacer, Eldarica), Atlas is **pure Horn**: no built-in arithmetic, bitvectors, or residual goals. Numeric and logical structure is encoded relationally (Peano arithmetic, Boolean circuits as clauses, etc.). Search heuristics carry domain knowledge rather than a separate theory layer. Rule order in the database and atom order within a rule body are **semantically irrelevant** — subgoals are conjunctive relations, not sequential program steps — so reordering clauses or body atoms does not change whether or which solutions Atlas finds. This order-independence is what makes Atlas a **pure relational** solver rather than a procedural backtracking interpreter.

Atlas is designed as a **fair search engine**: it does not fixate on a single derivation path. Bounded forward episodes, discretionary resolution via MCTS (or random sampling), and persistent CDCL learning distribute exploration across the decision space instead of committing exclusively to one branch. This is what enables **solution completeness** — if a satisfying model exists, Atlas will eventually find it.

---

## Solver specification

A conforming Atlas solver implements the following contract.

### Input

- **Database:** `.chc` file — Horn rules and facts defining the problem structure. See [Input format](#input-format).
- **Query:** conjunctive goal on the command line (`--goal "G1, G2, ..."`). This is the problem instance; variables may remain free.
- **Solver mode:** `horizon` | `ridge` | `basic`.

No theory constraints, no residual goals. The logic is purely relational unification over first-order terms. Declaration order of rules and of body atoms is not part of the semantics.

### Output

| Line | Meaning |
|---|---|
| `SOLVED` + bindings | A distinct satisfying substitution for the goal variables. |
| `REFUTED` | No further models exist in the explored decision space. |

- After one or more `SOLVED` lines, `REFUTED` means **enumeration complete**.
- `REFUTED` with no prior `SOLVED` means **unsatisfiable** (when refutation terminates).
- On infinite domains with no solution, termination with `REFUTED` is **not guaranteed**.

Bindings are printed in normalized form (full substitution applied to goal terms).

### Execution model

Search is split into two layers:

1. **`solver`** — lifecycle orchestration. One call to `solve()` runs: setup → **sim** → CDCL learn → teardown. The caller loops until `REFUTED` or external stop. Persistent state: CDCL avoidances, MCTS tree, `lineage_pool` (pinned lemmas), rule database, initial goals.

2. **`sim`** — one **forward CHC episode** with no search backtracking. Maintains an AND-OR proof tree (goals = AND nodes, candidate rules = OR branches). Terminates on:
   - **`solved`** — active goal set empty
   - **`conflicted`** — some active goal has zero candidates
   - **`depth_exceeded`** — resolution budget exhausted (`max_resolutions` counts all resolutions, including unit propagation)

After **every** sim termination, Atlas learns a **decision lemma** (minimal set of fixpoint decisions) into CDCL and never repeats that decision set. Solution-finding sims are learned too, enabling **model enumeration**.

### Resolution step (one sim iteration)

At each step:

1. **Unit propagation** — if a goal has exactly one active candidate, resolve it (forced).
2. **Fixpoint** — when the unit stack is empty after eliminations, pick a **decision**: `(goal, rule)` via MCTS or random.
3. **Elimination (before activation)** — CDCL unit avoidances, then MHU rebase; backlog suppresses candidate creation.
4. **Resolve** — commit bindings, activate rule body as subgoals, deactivate sibling candidates, record proof in `resolution_memory`.

**Decision** vs **unit** resolution: only fixpoint choices are recorded in `decision_memory` and CDCL lemmas; unit propagations are implied.

### Core mechanisms

| Mechanism | Role |
|---|---|
| **CDCL** | Persistent avoidances over decision sets; unit avoidances yield eliminations mid-sim. |
| **MHU** | Prospective head unifications per candidate; commit rebases remaining heads and prunes inconsistent candidates. |
| **Lineage pool** | Cons-hashed AND-OR derivation paths; pinned across sims for CDCL references. |
| **MCTS** | Discretionary goal/rule choice; tree persists across sims. Reward depends on solver mode. |

Implementation details: [docs/cdcl.md](docs/cdcl.md), [docs/mhu.md](docs/mhu.md), [docs/lineage.md](docs/lineage.md), [docs/solving-loop.md](docs/solving-loop.md).

---

## Architecture

```
.chc + --goal
      │
      ▼
  parser ──► db (rules) + initial_goal_exprs
      │
      ▼
  solver_driver ──► solver::solve()  [loop: setup → sim → learn → teardown]
      │
      ├── run_sim        forward episode (unit stack + decisions + eliminations)
      ├── cdcl           learn avoidances from decision lemmas
      ├── resolver       goal/candidate activation, subgoal generation
      └── normalizer     SOLVED output bindings
```

**State across sims:** CDCL store (trail-backtracked mutations), MCTS tree, pinned lineages, database, initial goals.

**Per-sim ephemeral:** active goals, candidate sets, bind map, expr pool allocations, decision/resolution memory, MHU heads (cleared at sim start).

---

## Solvers

All variants share CDCL, MHU, and the sim/solver split. They differ in discretionary resolution selection.

| Command | Decision policy | MCTS reward |
|---|---|---|
| `horizon` | MCTS over active goals (SRT) then candidates | **CGW** — cumulative grounded weight in `[0, 1]`; favors derivations close to facts |
| `ridge` | MCTS | **`−decision_count`** — favors shallow conflicts; broad CDCL lemmas (replaces implication-graph analysis) |
| `basic` | Uniform random | none |

`ridge-horizon` (combined reward) is planned.

See [docs/solvers.md](docs/solvers.md) for reward definitions and rationale.

---

## Input format

The `.chc` file is the **database** (rules and facts). The **query** is supplied separately via `--goal`.

**Facts:**
```prolog
parent(tom, bob).
nat(zero).
```

**Rules:**
```prolog
ancestor(X, Z) :- parent(X, Y), ancestor(Y, Z).
even(suc(suc(X))) :- even(X).
```

**Syntax:**
- Variables: uppercase (`X`, `Y`). Atoms/functors: lowercase.
- Lists: `[]`, `[H|T]`, `[a, b, c]`.
- Comments: `%` to end of line.
- **Query:** command line only (`--goal`), not embedded in `.chc` files. The database holds rules; the query is the problem.
- **Order:** rule order in the file and atom order in a rule body do not affect satisfiability or the set of solutions found.

**Example:**
```bash
./build/atlas horizon cli/examples/arithmetic/db.chc \
  --goal "even(X), lt(X, suc(suc(suc(suc(suc(suc(zero)))))))"
```

---

## Examples

Bundled under `cli/examples/`:

| Domain | Path | Sample goal |
|---|---|---|
| Ancestry | `ancestor/db.chc` | `ancestor(tom, X)` |
| Peano arithmetic | `arithmetic/db.chc` | `mul(suc(suc(zero)), suc(suc(zero)), R)` |
| List membership | `member/db.chc` | `member(X, [a, b, c])` |
| Binary trees | `trees/db.chc` | `tree(X), in(a, X)` |
| Reachability | `reachability/db.chc` | — |
| Boolean SAT | `sat/db.chc` | — |
| Expression synthesis | `expr/db.chc` | — |

---

## Building

**Dependencies:** C++20 (`g++`), GNU Make, ANTLR 4.10.1 runtime (`libantlr4-runtime-dev`), JAR in `tools/`.

```bash
make atlas                    # release → build/atlas
make core_debug && ./build/core_debug
make parser_debug && ./build/parser_debug
make cli_debug && ./build/cli_debug
```

| Target | Description |
|---|---|
| `atlas` | Release binary (`-O3`) |
| `*_debug` | Test binaries with `DEBUG_ASSERT` (`-DDEBUG -g`) |
| `*_debug_fast` | Debug asserts + `-O3` |
| `atlas_profile` | gprof build |

Layout: `core/` (engine), `parser/` (ANTLR `.chc` front-end), `cli/` (CLI11), `mcts/` (MCTS library), `docs/` (design notes).

---

## Documentation

| Topic | Doc |
|---|---|
| Solving loop, unit vs decision | [docs/solving-loop.md](docs/solving-loop.md) |
| Resolution, bind map, normalizer | [docs/resolution.md](docs/resolution.md) |
| CDCL, avoidances, lemmas | [docs/cdcl.md](docs/cdcl.md) |
| Multi-head unification | [docs/mhu.md](docs/mhu.md) |
| Lineage, pools | [docs/lineage.md](docs/lineage.md) |
| Manifest wiring | [docs/bootstrap.md](docs/bootstrap.md) |
| Testing | [docs/testing.md](docs/testing.md) |

---

## Status

Stable and actively developed. Core engine, CDCL, MHU, and MCTS variants (`horizon`, `ridge`) are implemented with unit and integration tests.

**Planned:** `ridge-horizon`; pre-sim refutation detection; functor-indexed rule lookup (currently full DB scan per goal); tighter integration of domain heuristics into search without a separate theory solver.
