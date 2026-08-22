<!--
  CHC solver (core) test coverage audit — pass 5 (integration realism).
  Authoritative rules: .cursor/rules/cpp-testing*.mdc

  Baseline at time of audit: 2324 tests / 198 suites, all green (22 skipped by
  design in runtime_test_suite's kind-specific tiers).

  Breadth is good; DEPTH and REALISM are not. The recurring failure mode is a
  slice that is "covered" by a test whose collaborators are mocks or fakes that
  cannot violate the contract being asserted. Ranked gap list:

  1. dbuct_frame_hub full undo round-trip.
     Untested: one push_solver_frame / pop_solver_frame cycle restoring all 16
     stores TOGETHER. integration/dbuct_sim.cpp uses FAKE mhu/cdcl; the
     unit dbuct_frame_hub.cpp drives real stores for goal_exprs only.
     Bug undetected: partial undo. Bind map restored but MHU heads / CDCL
     watchers / candidate frame offsets left stale after a camp, so the solver
     resumes a backtracked branch with poisoned state and silently returns a
     wrong answer (or misses solutions) rather than crashing.
     Action: NEW FILE integration/dbuct_frame_hub_round_trip.cpp.

  2. dbuct_joint_elimination_generator::constrain remove_head obligation.
     Untested against real components. Its entire reason to exist (vs the shared
     joint_elimination_generator) is "every CDCL yield must also drop that
     lineage's MHU head". unit/dbuct_joint_elimination_generator.cpp mocks BOTH
     generators, so the mock records the call but nothing proves the real MHU
     actually forgets the head.
     Bug undetected: an orphaned MHU head for a CDCL-eliminated lineage keeps
     participating in unification, producing contradictory or duplicated
     eliminations under camping.
     Action: NEW FILE integration/dbuct_joint_elimination_generator.cpp.

  3. dbuct_resolution_recorder five-oracle fan-out.
     Untested with real oracles. Unit test uses mocks; the aub+nearest_decision
     integration exists but without the recorder driving it.
     Bug undetected: a unit resolution recorded as a decision rotates the
     avoidance boundary, so CDCL learns at the wrong MCTS depth and prunes
     branches that still contain solutions.
     Action: NEW FILE integration/dbuct_resolution_recorder.cpp.

  4. unifier + dbuct_bind_map undo.
     integration/unifier_bind_map.cpp uses the plain NON-journaling bind_map.
     dbuct_bind_map's journal is only unit-tested with hand-written bind calls.
     Bug undetected: bindings made inside a camped frame survive the pop, so a
     backtracked variable stays bound and later unifications silently fail or
     succeed wrongly. Also covers whnf's path-compression write-back, which
     records a SECOND journal entry that must also unwind.
     Action: NEW FILE integration/dbuct_unifier_bind_map.cpp.

  5. resolver + real SRT deactivation seam.
     integration/srt_resolver_order_invariance.cpp mocks the resolver's
     deactivation tail; unit/resolver.cpp mocks the SRT side. Neither direction
     proves the seam.
     Bug undetected: goal expr / candidate rules leak after resolve, or a goal
     is removed from the active frontier but left in the SRT (ghost goal), so
     solution_detector never fires and the sim runs to depth_exceeded.
     Action: NEW FILE integration/resolver_srt_lifecycle.cpp.

  6. Conservation invariants only proven at depth 0 with a mocked resolver.
     integration/quell_work_conservation.cpp exercises ONLY initial goals
     (depth 0) and mocks the inner resolver; quell_goal_activator (the child
     depth/work path) is never in the loop. The sum invariant
     remaining_work == sum(goal_work_values) is never asserted directly.
     Bug undetected: incremental work drift at depth >= 1 — the MCTS reward is
     -remaining_work, so drift silently re-weights the whole search.
     Action: additional TEST_F in the two existing conservation files.

  7. Rollout policy under camping.
     All RP score tests use the NON-dbuct stores and run pre-simulation.
     dbuct_rp_heuristic_rollout_frame_hub is mock-only.
     Bug undetected: rollout scores from a camped subtree leak into the parent
     frontier after terminate, permanently biasing goal selection.
     Action: NEW FILE integration/dbuct_rp_rollout_camping.cpp.

  8. dbuct_* stores lack their non-framed twins' assertion contracts.
     goal_depths.cpp has 7 tests (incl. GetOnUnknownGoalThrows, SecondSetThrows,
     EraseOnUnknownGoalThrows); dbuct_goal_depths.cpp has 4 and none of those.
     Same for dbuct_goal_work_values. dbuct_remaining_work never checks a
     negative balance or >2 nested frames.
     Bug undetected: a framed store silently accepting a duplicate set (instead
     of asserting) logs a bogus insert action whose undo erases a live entry.
     Action: additional TEST_F in the existing dbuct_* store files.

  9. run_sim / set_up_sim / tear_down_sim have NO unit file.
     run_sim owns the entire sim_termination decision (conflicted on failed
     initial activation, solved, depth_exceeded, conflicted on failed resolve)
     and the unit-goal-before-decision-generator ordering. It is only ever
     exercised through manifests, where a termination bug looks like "solver
     got the wrong answer" with no localization.
     Bug undetected: depth_exceeded reported as conflicted (or vice versa) makes
     the solver claim UNSAT on a resource limit.
     Action: NEW FILES unit/infrastructure/{run_sim,set_up_sim,tear_down_sim}.cpp.

 10. expr / framed_expr / lineage operator<=> untested.
     Only the hash functors are tested. expr_pool interns on ordering, and
     framed_expr snapshots are the payload of goal_expr_erase undo actions.
     Bug undetected: a wrong ordering breaks hash-consing — two structurally
     equal exprs get distinct interned pointers, so pointer-keyed maps silently
     miss and unification compares the wrong terms.
     Action: NEW FILES unit/value_objects/{expr,framed_expr,lineage}.cpp.

 11. quell / horizon frame hubs never forward base yields.
     dbuct_rp_heuristic_rollout_frame_hub has PopForwardsBaseYields /
     PopForwardsMultipleBaseYields; the quell and horizon hubs only test call
     ordering against an EMPTY base pop.
     Bug undetected: a hub that drops (or duplicates) the CDCL eliminations the
     base pop yields loses learned clauses at every backtrack.
     Action: additional TEST_F in the two existing hub unit files.

 12. Solver-family math edges.
     goal_work_function never asserts f(0) or k==0; ridge_reward never checks
     count()==0 or monotonicity (the actual search objective); quell_reward
     never covers a negative balance; horizon_goal_activator only covers a
     two-atom body.
     Action: additional TEST_F in the existing unit files.

  FOUND BY PASS 5 — a scoping rule for the conservation invariants, NOT a bug:

  * Post-conflict state is undefined; do not assert on it.
    See "Behaviors discovered while writing the pass 5 tests" below.

  NOT converted to tests (production questions, deliberately left alone per
  .cursor/rules/cpp-testing-policy.mdc — do not fix bugs found while testing):
  - horizon_goal_activator::activate computes get(parent) / double(g) where
    g = body.size(). For a fact (g == 0) this divides by zero. Production
    reaches this only if a fact is routed through the child activator rather
    than horizon_resolver's accumulate path; the contract for g == 0 is
    undefined, so no test asserts a value for it.
  - get_unit_resolution calls .front() on the candidate set with no emptiness
    check. Calling it on an exhausted goal is UB, so no test invokes it.
-->

# Core test coverage audit (pass 5 — integration realism)

See the comment block above for the authoritative pass 5 gap list. Summary of
what pass 5 adds:

| Group | Where | Tests | Theme |
|-------|-------|-------|-------|
| A | `integration/dbuct_frame_hub_round_trip.cpp` (new) | 6 | All stores undo together |
| B | `integration/dbuct_joint_elimination_generator.cpp` (new) | 4 | `remove_head` vs real MHU |
| C | `integration/dbuct_resolution_recorder.cpp` (new) | 3 | Five-oracle fan-out |
| D | `integration/dbuct_unifier_bind_map.cpp` (new) | 4 | Binding undo |
| E | `integration/resolver_srt_lifecycle.cpp` (new) | 4 | Resolver/SRT seam |
| F | existing conservation files | 4 | Depth >= 1, sum invariant |
| G | `integration/dbuct_rp_rollout_camping.cpp` (new) | 3 | RP scores under camping |
| H | existing `dbuct_*` store files | 7 | Parity with non-framed twins |
| I | existing solver-family files | 8 | Math and boundary values |
| J | `unit/infrastructure/{run_sim,set_up_sim,tear_down_sim}.cpp` (new) | 7 | Untested shared bases |

Group J also retires `unit/infrastructure/sim.cpp`. That file built all three
bases as real SUTs side by side, which is what hid them from a per-component
search and breaks the one-real-implementation rule in
`.cursor/rules/cpp-testing.mdc`. Every case it held moved into the file for the
base it actually exercised; nothing was dropped.

| K | `unit/value_objects/{expr,framed_expr,lineage}.cpp` (new) | 4 | `operator<=>` contracts |
| L | existing quell/horizon hub files | 2 | Base yield forwarding |

Mock specificity applied throughout: `.Times(1)` only on mutations, emissions
and allocations; `.Times(AtLeast(1))` or bare `ON_CALL` on getters and const
reads. Integration tests assert observable end state, never call counts.

## Behaviors discovered while writing the pass 5 tests

Two things turned up here. The first is a scoping rule for what the conservation
invariants actually cover. The second is a load-bearing assumption that nothing
else records.

**The conservation invariants hold only up to a terminating moment.**
The horizon invariant is `cumulative_grounded_weight + sum(active goal weights)
== total`, and the quell one is `remaining_work == sum of the active frontier's
work`. Both hold at every point in time BEFORE termination is reached. They say
nothing about the state left behind by a conflict or a solution.

That matters because a conflict is routine, not exceptional.
`activate_goal_candidates` returns `false` in exactly one place — when
`conflict_detector` finds the goal has zero surviving candidate rules — and
`subgoals_activator` activates each body goal BEFORE checking it:

```
const goal_lineage* gl = make_goal_lineage_.make_goal_lineage(rl, body_idx);
goal_activator_.activate(gl);
if (!activate_goal_candidates_.activate_goal_candidates(gl))
    return false;
```

So resolving `f :- g, h.` where nothing matches `g` weights `g` with half of
`f`'s share, then discovers `g` is a dead end and returns, never creating `h`.
The frontier momentarily reads 1.5 against a total of 1.0, `srt_subgoals_activator`
never reaches `link_srt_goal_batch_parent`, and `in_flight_` is left populated.
None of that is a defect: the caller immediately abandons the branch — `run_sim`
turns the false resolve into `sim_termination::conflicted` and `tear_down_sim`
clears every store, or under dbuct the frame pop rewinds it.

Consequence for tests: assert the invariants only on states reached by
successful activation and resolution. Four tests that asserted post-conflict
state were removed rather than weakened, since a weakened assertion on an
undefined state still implies the state is defined.

**`expr::functor` ordering compares argument POINTERS, not structure.** `args`
is `std::vector<const expr*>` and `operator<=>` is defaulted, so ordering is by
argument identity. This is only equivalent to structural ordering because
`expr_pool` interns bottom-up — children are interned before the parents that
reference them. If anything ever interns a parent before its children, two
structurally identical exprs get distinct pool entries and unification will
decide two identical goals differ. `unit/value_objects/expr.cpp` pins the actual
pointer-identity contract and records why the interning order matters.

---

# Core test coverage audit (pass 4 — compile-time polymorphism)

## Architectural change summary (pass 4)

All `i_*` abstract base classes (except `i_backtrackable`) have been removed. The codebase
now uses **compile-time polymorphism via C++ templates** exclusively:

- **Missing-dependency bugs are now compile errors.** A class that forgets to wire a
  dependency will fail to instantiate, not throw at runtime.
- **Mock injection via template parameters** is the new contract-test mechanism.
  Each unit test declares `using TestSUT = sut<Mock1, Mock2, ...>` and constructs the
  SUT directly with mock references. No `locator`, no `bind_as`, no interface headers.
- **`locator` has been removed** from production code. Manifests (`basic_manifest`,
  `ridge_manifest`, `horizon_manifest`) are flat composition roots with `using` type
  aliases for complex instantiations.
- **Integration tests** construct the full stack via manifests or directly via type
  aliases. The sim integration test uses `MockGenerateDecision` (a free-standing struct,
  no base class) as the only scripted collaborator.

---

# Core test coverage audit (pass 3)

## Executive summary

| Metric | Notes |
|--------|-------|
| Prior pass | 363 tests; shallow arity / multi-branch unit gaps largely closed |
| Pass 3 focus | Full **sim** stack integration (real wiring, scripted `i_generate_decision`), joint/MCTS/state_machine gaps |
| Explicitly out of scope | Full **solver** integration (`basic_manifest::entry()` loop), `basic_manifest` smoke |

---

## Coverage map (infrastructure)

| Component | Coverage | Gap severity |
|-----------|----------|--------------|
| `sim` | Unit: termination, mocks, ordering (11 tests) | **High** — no integration with real resolver/activators/elim/joint |
| `basic_manifest` | **Untested** | Low for unit — wiring-only; bugs surface via sim/solver integration |
| `solver` | Unit only (mocked `i_run_sim`) | Deferred per task (no full solver integration) |
| `mcts_decision_generator` | Singleton goal+rule only | **Medium** — multi-goal / multi-candidate Cartesian product |
| `joint_elimination_generator` | CDCL→MHU order, one-sided empty | **Low** — both streams empty |
| `state_machine<void>` | Suspend/resume only | **Low** — exception propagation on `resume()` |
| `random_decision_generator` | Multi goal/rule | OK |
| Remaining value-object smoke (`expr` `<=>`) | None | Negligible |

---

## Untested / under-tested behaviors

### 1. `sim::run` with real orchestration slice

- **What:** `set_up` → loop (`solution_detector`, `next_resolution`, `joint.constrain`, `elimination_router`, `resolver`) → `tear_down` on real `trail`, `active_goals`, `resolver`, `joint_elimination_generator`, etc.
- **Why it matters:** Unit tests mock every collaborator; regressions in **wiring** (wrong locator binding, missing clear on tear_down, resolution order) only appear when real components run. Example bug: `resolver` succeeds but `active_goals` not updated → never `solved`.
- **Action:** **New file** `core/test/integration/sim.cpp`, `TEST_F(SimIntegrationTest, …)`. Real stack from `basic_manifest` minus `solver` / `random_decision_generator`; **GMock** `MockGenerateDecision` with scripted `WillOnce` sequence (contract: observable termination, not call counts on getters).

### 2. `sim::run` `depth_exceeded` with real resolver

- **What:** One resolution step leaves active goals; `max_resolutions == 1` → `depth_exceeded`.
- **Why it matters:** Distinguishes resource limit from `conflicted` / `solved`; unit test already mocks resolver.
- **Action:** Same integration file, `TEST_F`.

### 3. `goal_deactivator` / `sim_termination::solved` (bug surfaced by integration)

- **What:** After resolving a fact clause, `active_goals` may still be non-empty because `goal_deactivator::deactivate` does not call `erase_active_goal`.
- **Why it matters:** `solution_detector` never reports solved; sim loops until `depth_exceeded`.
- **Action:** Failing contract test deferred; integration asserts `depth_exceeded` + resolution count instead until fixed.

### 4. `mcts_decision_generator::generate` with multiple goals and candidates

- **What:** `active_goals_size > 1`, multiple rule ids; output must be `make_resolution_lineage(chosen_gl, chosen_r)` for some valid pair.
- **Why it matters:** Off-by-one in choice vector building breaks search silently.
- **Action:** **Additional `TEST_F`** in `mcts_decision_generator.cpp`; `Invoke` on `make_resolution_lineage` (not exact `Times(1)` on `active_goals_size` — idempotent read).

### 5. `joint_elimination_generator::constrain` when CDCL and MHU both empty

- **What:** No yields; stream completes with `done()` and no value.
- **Why it matters:** Empty composition must not hang or yield spurious eliminations.
- **Action:** **Additional `TEST_F`** in `joint_elimination_generator.cpp`.

### 6. `state_machine<void>::resume` exception propagation

- **What:** Coroutine throws → `resume()` rethrows; repeated `resume()` still throws (matches `T` specialization).
- **Why it matters:** Void machines used for side-effect-only flows; swallowed exceptions → undefined elimination state.
- **Action:** **Additional `TEST_F`** in `state_machine.cpp`.

### 7. `basic_manifest` construction / `entry()`

- **What:** Locator binds all services; `entry()` returns `solver`.
- **Why it matters:** Missing `bind_as` → runtime throw on first solve.
- **Action:** Deferred (solver integration out of scope); optional future smoke with tiny `db`.

---

## Mock specificity (pass 3)

| Call | Contract? | Expectation style |
|------|-----------|-------------------|
| `MockGenerateDecision::generate` | Yes — which resolution is selected | `WillOnce` / sequence per test |
| `make_resolution_lineage` in MCTS test | Yes — pair must be in active×candidates | `Invoke` + `EXPECT_TRUE` on valid pair |
| `active_goals_size`, `iterate_active_goals` in MCTS | No — idempotent reads | `WillOnce` / `WillRepeatedly`, not exact loop counts |
| `solution_detector::detect` in sim integration | No | Default real implementation |
| `tear_down` clears (`pop`, `clear_*`) | Yes — each clear runs once per tear_down | `Times(1)` only if asserting tear_down in mock-heavy unit test; integration asserts **state** instead |

---

## Pass 3 tests to add

| File | Tests |
|------|-------|
| `test/integration/sim.cpp` | `RunRecordsResolutionForScriptedFactDecision`, `RunReturnsDepthExceededWhenSubgoalRemains` |
| `test/unit/infrastructure/mcts_decision_generator.cpp` | `GeneratePicksAmongMultipleGoalsAndCandidates` |
| `test/unit/infrastructure/joint_elimination_generator.cpp` | `ConstrainYieldsNothingWhenBothStreamsEmpty` |
| `test/unit/infrastructure/state_machine.cpp` | `VoidMachineExceptionPropagatesOnResume` |
