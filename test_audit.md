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
