<!--
  CHC solver (core) test coverage audit — second pass (depth / arity / edge cases).
  Authoritative rules: docs/testing.md
-->

# Core test coverage audit (pass 2)

## Changes since pass 1

- **Removed** `core/hpp/infrastructure/backtrackable_set_extract.hpp` — unused anywhere in the repo (only self-references; never instantiated).
- **Added** 20+ tests targeting shallow coverage: higher-arity `expr` trees, multi-body rules, multi-choice decision generators, OOB accessors, and richer WHNF/print paths.

## Executive summary

| Metric | Value |
|--------|-------|
| Tests (`core_debug`) | 344 → **363** (after pass 2) |
| Deleted dead code | `backtrackable_set_extract.hpp` |
| Pass-2 focus | Expression arity ≥ 3, nested functors, multi-body resolver, multi-candidate decisions |

---

## Shallow-test patterns found (pass 2)

| Area | Previous gap | Risk if regressed |
|------|----------------|-------------------|
| `expr_printer` | Only unary/binary cons and 2-arg `f` | Wrong output for real CHC terms (3+ args, long lists) |
| `expr_pool` | No ternary intern/import | Canonicalization breaks on typical clause heads |
| `bind_map` | Nullary/`f(var)` only | WHNF on composite representatives wrong |
| `overlay_bind_map` | Binary functor at most | Layered WHNF on multi-arg spines |
| `goal_activator` | Single `body.at(0)` | Wrong subgoal copied for multi-clause rules |
| `resolver` | One body literal | Skips activations / ordering on multi-goal rules |
| `random_decision_generator` | Singleton goal + rule | Silent break when search has real branching |
| `db` / `initial_goal_exprs` | No OOB | Undefined behavior vs `at()` contract |
| `resolution_memory` | Never asserts `get_resolution_count` | Decision loop exit wrong |
| `solver` | No `depth_exceeded` yield | Driver mishandles resource limits |
| `candidate_activator` | Atomic heads only | MHU sees wrong shape for functor heads |
| `mhu` (integration) | Nullary head collisions only | Arity-specific unify/rebase bugs |

---

## Pass 2 tests added (by file)

| File | New tests |
|------|-----------|
| `expr_printer.cpp` | `PrintTernaryFunctor`, `PrintFourElementList`, `PrintNestedFunctorArgs` |
| `expr_pool.cpp` | `TernaryFunctorInternedTwiceReturnsSamePointer`, `ImportTernaryFunctorInternsEachArg` |
| `bind_map.cpp` | `WhnfBoundToBinaryFunctor`, `WhnfBoundToTernaryFunctor` |
| `overlay_bind_map.cpp` | `WhnfTernaryFunctorDelegatesToRemote`, `WhnfVarBoundToTernaryFunctorLocally` |
| `goal_activator.cpp` | `ActivateUsesBodyIndexForSubgoalExpr` |
| `resolver.cpp` | `ActivatesEveryBodySubgoal` |
| `random_decision_generator.cpp` | `GeneratePicksAmongMultipleCandidates`, `GeneratePicksAmongMultipleActiveGoals` |
| `db.cpp` | `GetRuleIdOutOfRangeThrows` |
| `initial_goal_exprs.cpp` | `GetOutOfRangeThrows` |
| `resolution_memory.cpp` | `GetResolutionCountTracksInsertions` |
| `solver.cpp` | `YieldPropagatesDepthExceededTermination` |
| `candidate_activator.cpp` | `AcceptedTernaryHeadPassedToMhu` |

**Note:** MHU `try_add_head` rejects functor heads whose args contain the goal variable (occurs-check path); ternary collision on same rep is not integration-tested here — nullary `f`/`g` collision tests remain the MHU constrain contract.

---

## Remaining backlog (not in pass 2)

| Item | Why deferred |
|------|----------------|
| `mcts_decision_generator` multi-branch | MCTS tree state; needs careful fixture beyond singleton choices |
| `sim::run` full elimination + multi-step | Heavy mock SM wiring; partial coverage from pass 1 |
| `state_machine<void>` exception | Low risk; `void` resume API differs from `T` |
| `joint` both streams empty | Edge case; low frequency in production |
| Value-object smoke (`expr` `<=>`) | No behavior beyond structural |

---

## Mock specificity (pass 2)

- **Multi-candidate / multi-goal decisions:** `make_resolution_lineage(goal, rule)` validated with `Invoke` — contract is “choice ∈ Cartesian product”, not a fixed seed-dependent pair.
- **Side effects** (`activate`, `set`, `route`, `try_add_head`): `Times(1)` when the test outcome depends on that call happening.
- **Reads** (`get_rule`, `detect`, `count`): `WillOnce` / `WillRepeatedly`; no spurious exact totals across loops.

---

# Pass 3 audit — DBUCT nearest-decision & avoidance-unit-boundary

Scoped audit of two previously **untested** headers (no test file existed for either):

- `core/hpp/infrastructure/dbuct_nearest_decision.hpp`
- `core/hpp/infrastructure/dbuct_avoidance_unit_boundary.hpp`

Both are header-only templates with duck-typed collaborators (`ILogTrailAction::log`, `IGetNearestDecision::get_nearest_decision`, `IGetFrameCount::depth`), so they are tested with GMock doubles wired directly (no `i_*` base exists). They link standalone against gtest/gmock — no dependency on the (currently broken) `libatlas_core`.

## `dbuct_nearest_decision` — gap table

| Method / behavior | Coverage before | Why it matters (bug caught) | Action |
|-------------------|-----------------|-----------------------------|--------|
| `get_nearest_decision(nullptr)` (seed `{nullptr->nullptr}`) | none | If the seed is dropped, every root-level unit lookup throws or misreports; the whole avoidance chain is built on this base case | new file, new TEST_F |
| `note_decision_resolution` -> self-map | none | A decision must be its own nearest decision; if broken, avoidance lemmas attribute units to the wrong decision | new TEST_F |
| `note_unit_resolution` inherits grandparent's nearest decision | none | The core propagation rule; a wrong parent hop silently mis-attributes units across decision boundaries | new TEST_F |
| Nearest-decision propagation down a unit chain | none | Guards that inheritance is transitive across successive units (deep chains are the common case in resolution) | new TEST_F |
| Root unit (`grandparent == nullptr`) inherits `nullptr` | none | Distinguishes "no decision above" from a real decision; off-by-one here poisons boundary math | new TEST_F |
| `note_unit_resolution` with unrecorded grandparent (`map::at` precondition) | none | Documents the ordering precondition; a missing record throws rather than silently inserting garbage | new TEST_F (`EXPECT_THROW`) |
| Each `note_*` journals to the trail (backtrackable) | none | Without a journal entry, backtracking can't undo the insert -> stale nearest-decision after pop | new TEST_F, `Times(AtLeast(1))` (journaling is the contract; exact count is impl detail) |

## `dbuct_avoidance_unit_boundary` — gap table

| Method / behavior | Coverage before | Why it matters (bug caught) | Action |
|-------------------|-----------------|-----------------------------|--------|
| Initial state (boundary 0, ultimate/penultimate null) | none | Fresh solver state must not report a spurious boundary | new file, new TEST_F |
| **Invariant pt 1:** after 1 decision, boundary still 0 | none | The boundary must lag; a premature non-zero boundary over-prunes the avoidance region | new TEST_F |
| **Invariant pt 2:** after 2nd decision, boundary == prior decision's frame index (one behind) | none | This is THE contract; an off-by-one makes avoidance lemmas cut at the wrong frame | new TEST_F |
| Overwrite branch (new decision extends current ultimate's chain) | none | When `nearest(rl grandparent) == ultimate`, boundary/penultimate must NOT rotate; wrong branch corrupts the lag | new TEST_F |
| Rotate branch driven by REAL nearest-decision map | none | Unit test stubs the branch; only integration proves the real map selects rotate vs overwrite | new integration file |
| `log_decision` journals to the trail | none | Boundary/ultimate mutations must be undoable on pop | new TEST_F, `Times(AtLeast(1))` |

## Out of scope / backlog

| Item | Why deferred |
|------|--------------|
| Real `trail` + `push`/`pop` undo survival for both structures | Per `docs/testing.md`, backtracking survival belongs in a dedicated integration slice with the real `trail`; the two-component integration here mocks the trail because the interaction under test is the nearest-decision -> boundary branch, not undo |
| `dbuct_avoidance_unit_boundary::log_decision` with a null-parent decision | Code dereferences `rl->parent->parent`; a root decision with null parent is not a valid input in production (decisions always sit under a goal), so it is a precondition, not a tested path |
