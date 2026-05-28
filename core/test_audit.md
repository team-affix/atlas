<!--
  CHC solver (core) test coverage audit — pass 3 (sim integration, deferred backlog).
  Authoritative rules: docs/testing.md (repo root)
-->

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
