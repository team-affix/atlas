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

---

# Pass 4 audit — DBUCT CDCL elimination generator

Scoped audit of the one component the delayed-backtracking rewrite touched most:
`core/hpp/infrastructure/dbuct_cdcl_elimination_generator.hpp`. It was **entirely
untested** (the existing `cdcl_elimination_generator` unit/integration files cover
the restart-based variant, whose API — `learn(lemma) -> optional`, `cleanup()` — is
unrelated to the DBUCT variant's `learn()` / `push_frame()` / `pop_frame()` frame
model). No `dbuct_cdcl_*` test file existed.

Two files added, both compiled/run **standalone** against gtest/gmock (no dependency
on the currently-broken `libatlas_core`), matching the pass-3 `dbuct_*` pattern:

- `core/test/unit/infrastructure/dbuct_cdcl_elimination_generator.cpp` — SUT real, all 5 collaborators mocked.
- `core/test/integration/dbuct_cdcl_elimination_generator.cpp` — SUT + real `dbuct_avoidance_unit_boundary` (which supplies `IGetUnitBoundary` / `IGetUltimateDecision` / `IGetPenultimateDecision` in production), everything else mocked.

**Observability:** the SUT has no getters and `learn()` / `push_frame()` are `void`,
so every assertion is made only against the `resolution_lineage*` eliminations
yielded by `constrain(rl)` and `pop_frame()` (plus collaborator call contracts). No
internal state (`frame_stack_`, `watched_goals_`, `avoidances_`, member order) is
inspected.

## Compile-blocking bugs (fixed with explicit approval)

The header did not compile *at all* — even a standalone template instantiation
failed (`-fsyntax-only`). Three one-line typos, fixed at the user's direction so the
tests can run (no other source changes):

| Loc | Was | Fix | Effect if unfixed |
|-----|-----|-----|-------------------|
| ctor | `frame_stack_(1)` | seed one root frame via `frame_stack_.push(frame{})` | `std::stack` has no count ctor → hard compile error for any instantiation |
| `learn()` | `frame_stack_.top().avoidance_actions.emplace_back(...)` | `...raised_unit_avoidance_lump.emplace_back(...)` | no member `avoidance_actions` → compile error |
| `visit_avoidance()` | `frame_stack_.back().actions` | `frame_stack_.top().actions` | `std::stack` has no `back()` → compile error |

## Gap table — `dbuct_cdcl_elimination_generator`

| Method / behavior | Coverage before | Why it matters (bug caught) | Action |
|-------------------|-----------------|-----------------------------|--------|
| `constrain` on fresh generator | none | Empty-store lookup must be a no-op, not a crash/spurious elimination | new file, new TEST_F |
| `pop_frame` of an empty child frame | none | The frame journal must unwind cleanly with nothing learned | new TEST_F |
| `learn` empty lemma (0 resolutions) | none | A no-op lemma must record neither avoidance nor raised unit; else pop emits phantom eliminations | new TEST_F |
| `learn` does not watch a fresh avoidance | none | Freshly-learned conflicts are raised-unit, not armed; watching early would double-fire | new TEST_F |
| `pop_frame` still-unit branch emits `members[watcher_a]` | none | The core forced-elimination emission; if wrong, learned conflicts never propagate on ascent | new TEST_F |
| still-unit avoidance bubbles up across successive pops | none | Deferred unit must survive multiple backsteps until it crosses its boundary | new TEST_F |
| `pop_frame` past-boundary branch arms (no emit) then `constrain` forces | none | The emit-vs-arm decision is THE delayed-backtracking contract; an off-by-one over/under-prunes | new TEST_F |
| `constrain` binary avoidance forces the other watcher | none | Two-watched-literal propagation base case | new TEST_F |
| fired avoidance does not refire on the same goal | none | Terminal avoidance must unwatch; else infinite/duplicate eliminations | new TEST_F |
| `constrain` mutually-exclusive sibling satisfies + unwatches | none | Wrong-rule commit means the clause can't be violated on this branch; must go silent | new TEST_F |
| `constrain` watcher migration to an unassigned member (3-member) | none | Watched-literal maintenance; a stuck watcher yields false forces | new TEST_F |
| `constrain` consistent tail member forces immediately | none | `scan` reaching `members.size()` = forced | new TEST_F |
| `constrain` exclusive tail member satisfies (`scan == SIZE_MAX`) | none | `scan`'s satisfied path; a wrong return misclassifies the clause | new TEST_F |
| `pop_frame` undoes an unwatch (re-arm) | none | Backtracking must restore a consumed avoidance; else it is lost for future descents | new TEST_F |
| `pop_frame` restores the visited/unvisited partition after a watcher migration | none | Backtracking must reopen members that were committed inside the frame, even though the migrated watcher legitimately stays put | new TEST_F (was B2; **now passing** after the fix below) |
| single-resolution lemma "floats" as an elimination | none | The `size==1` float path documented in `learn()` | new TEST_F (was B1; **now passing** after the fix below) |
| single-resolution unit stays unit across multiple pops (never arms) | none | Locks the invariant the `SIZE_MAX` second-watcher poison relies on: a unit has boundary 0 so it can never reach the arm branch | new TEST_F |
| `learn` consults the unit boundary per stored conflict | none | The boundary read is what drives emit-vs-arm; omitting it breaks the lag | new TEST_F, `Times(AtLeast(1))` (exact count is impl detail) |

## Gap table — integration with `dbuct_avoidance_unit_boundary`

| Behavior | Coverage before | Why it matters | Action |
|----------|-----------------|----------------|--------|
| Real lagging boundary keeps a 2-member conflict unit on pop (emits ultimate) | none | Proves the real ultimate/penultimate/boundary — not a stub — feed `learn` and the emit decision | new integration file, new TEST_F |
| Deep boundary: emit at depth >= boundary, then arm once popped above it, then `constrain` propagates | none | End-to-end emit->arm->force transition driven by the real component's one-decision lag; mocks alone cannot prove the boundary arithmetic lines up | new TEST_F |

## Behavioral bugs

- **B1 — single-resolution lemma threw instead of floating (FIXED).** Originally
  `learn()` recorded a `raised_unit_avoidance{id, boundary}` for a `size == 1` lemma
  but returned before storing any `avoidances_[id]` entry, so both `pop_frame`
  branches faulted on `avoidances_.at(rua.id)` with `std::out_of_range`. Fixed (at
  the user's direction) by storing a degenerate single-member avoidance
  `avoidance{{member}, /*watcher_a=*/0, /*watcher_b=*/SIZE_MAX}`: the still-unit pop
  branch reads only `watcher_a` (member 0) and floats the elimination. `watcher_b` is
  a `SIZE_MAX` poison because a unit has no second literal; it is only ever read by
  `link_watchers` in the arm branch, which a unit cannot reach — its `unit_boundary`
  is 0 and `frame_stack_.size() < 0` is impossible for an unsigned depth. That
  never-arm invariant is now guarded by `DEBUG_ASSERT(...watcher_b_pos != SIZE_MAX)`
  in `pop_frame`'s arm branch, so a future regression (a unit fed a nonzero boundary)
  fails loud rather than re-throwing `out_of_range`. Now passing: unit
  `SingleResolutionLemmaFloatsToPopAsElimination` and
  `SingleResolutionLemmaStaysUnitAcrossMultiplePops`.

- **B2 — `pop_frame` strands a reopened member on the visited side after a watcher
  migration (FIXED).** Note the contract here is *not* "restore the exact pre-frame
  watchers": keeping a watcher on its migrated-to member is an intended efficiency
  boost, and watcher identity is not observable contract. What must be preserved is
  the visited/unvisited partition — a member committed inside the frame must be open
  again after the pop, and every watcher must sit on an unvisited member.
  `visit_avoidance` logged `avoidance_watcher_update{id, a_fired, fired_pos}` *after*
  assigning `fired_pos = hit` (`fired_pos` being a reference alias onto
  `watcher_a_pos`/`watcher_b_pos`), so the journal stored the **new** position, not
  the pre-migration one. `undo_action` then swapped `members[pos]` with itself and
  re-assigned the same position — a no-op. The migrated watcher was left in place
  (fine), but the member that had become unvisited (`ult`) was stranded at its old
  slot, *left of* both watcher positions, where `scan` (which only looks past
  `max(watcher_a, watcher_b)`) never revisits it. Committing the migrated member then
  spuriously forced the other watcher as if the clause were unit. Fixed by capturing
  the previous slot before overwriting the alias
  (`const size_t prev_watcher_pos = fired_pos; fired_pos = hit;` and journaling
  `prev_watcher_pos`); `undo_action`'s swap now relocates the reopened member out past
  the watchers (`[ult, pen, x]`→`[x, pen, ult]`, watchers `{x, pen}`), so `scan` sees
  `ult` as open again and no spurious force occurs. Now passing: unit
  `PopRestoresUnvisitedPartitionAfterWatcherMigration`.

## Mock specificity (pass 4)

- **Getters/reads** (`try_get`, `get_unit_boundary`, `get_ultimate_decision`,
  `get_penultimate_decision`, `depth`, `get_nearest_decision`): `ON_CALL` defaults
  (or `Times(AtLeast(1))` only where the read *must* happen, e.g. `learn` consulting
  the unit boundary). No `.Times(1)` change-detectors on idempotent reads.
- **Lemma supply** (`derive_decision_lemma`): `WillOnce(Return(...))`, retiring so
  sequential `learn()` calls each get their own lemma; count matches the number of
  `learn()` calls the test actually makes.
- **Assertions are outcome-based** — the yielded eliminations from `constrain` /
  `pop_frame` — never internal watcher positions or member order (satisfies "test
  behavior, not implementation").
