# Testing

This repo uses **Google Test** and **Google Mock** for C++ tests. The important split is between **unit** and **integration** tests: they answer different questions and follow different dependency rules.

## Layout

Each package that has tests follows the same directory pattern:

| Path | Purpose |
|------|---------|
| `<package>/test/unit/` | Unit tests — one real component per test file |
| `<package>/test/integration/` | Integration tests — several real components wired together |
| `<package>/test/main.cpp` | GTest entry point for that package’s debug test binary |

For example, `core` uses `core/test/unit/`, `core/test/integration/`. Other packages (`parser`, `cli`, …) use the same layout under their own `test/` tree when tests are added.

Each package’s debug build links its tests into a dedicated binary (e.g. `make core_debug` → `./build/core_debug`).

## Mocks (Google Mock)

All collaborators in tests must be **GMock mocks** — types that inherit the relevant `i_*` interface and use `MOCK_METHOD`. Do **not** add hand-written `fake_*`, `recording_*`, or `noop_*` test doubles that duplicate production behavior.

### How to mock

1. Define a mock next to the test (or in a shared header only if many files need the same mock):

```cpp
struct MockBindMap : public i_bind_map {
    MOCK_METHOD(void, bind, (uint32_t, const expr*), (override));
    MOCK_METHOD(const expr*, whnf, (const expr*), (override));
};
```

2. Use production **`struct`**, not `class`, for mocks and test fixtures (match repo style).

3. Wire the SUT with the mock, and call through the interface when useful:

```cpp
NiceMock<MockBindMap> local;
NiceMock<MockBindMap> remote;
overlay_bind_map obm{local, remote};
i_bind_map& sut{obm};
```

4. Specify behavior with **`EXPECT_CALL`** (strict — unexpected calls fail) or **`ON_CALL`** (default stubs, optional calls):

```cpp
EXPECT_CALL(local, bind(0u, &func));
EXPECT_CALL(remote, bind).Times(0);
EXPECT_EQ(sut.whnf(&var0), &func);
```

5. Control returns with **`WillOnce`**, **`WillRepeatedly`**, **`Return`**, **`ReturnRef`**:

```cpp
EXPECT_CALL(rules, size()).WillOnce(Return(0));
EXPECT_CALL(ggcr, get(&gl)).WillOnce(ReturnRef(rules));
```

### `NiceMock` vs `StrictMock`

| Wrapper | Use when |
|---------|----------|
| `NiceMock<MockT>` | Most tests: uninteresting calls are ignored (e.g. interning tests that never touch `trail`) |
| `StrictMock<MockT>` | You need to prove an exact sequence or count of calls (e.g. “first intern logs once”, “repeat intern does not log again”) |
| Plain `MockT` | Same as strict — prefer `NiceMock` unless you are asserting specific calls |

### What not to do

- **No duplicate implementations** of `i_trail`, `i_bind_map`, etc. in test code “because the real one is simple.” If undo must actually run, that is an **integration** test with the real type; if you only care that the SUT called `log`, use `EXPECT_CALL` on `MockTrail`.
- **No mutable recording structs** (fields like `last_gl`, `std::set removed`) when `EXPECT_CALL` can assert the same contract.
- **No real collaborators in unit tests** except the SUT — e.g. do not use real `trail` in `expr_pool` unit tests; use `MockTrail` and move push/pop survival cases to integration.

### Overloaded virtual methods

When an interface overloads a method (e.g. `i_lineage_pool::pin`), mock with distinct names and forward:

```cpp
MOCK_METHOD(void, pin_goal, (const goal_lineage*), ());
void pin(const goal_lineage* gl) override { pin_goal(gl); }
```

### `unique_ptr` parameters

Use double parentheses in `MOCK_METHOD`:

```cpp
MOCK_METHOD(void, log, ((std::unique_ptr<i_backtrackable>)), (override));
```

## Unit tests

A **unit test** verifies the behavior of **one** class or function — the **unit under test (SUT)**. Every collaborator is a **mock**; the SUT is the only real production type under test.

### Rules

1. **Exactly one real implementation** of production code in each test (the SUT).  
   - Example: `overlay_bind_map` → real `overlay_bind_map`, `MockBindMap` for local and remote.  
   - Example: `bind_map` → real `bind_map` only.

2. **Mock every dependency** through its `i_*` interface with GMock.

3. **Do not wire multiple real infrastructure components** in a unit test.  
   If the test needs real `bind_map` and real `overlay_bind_map` together, it belongs in **integration**.

4. **Prefer observable outcomes**; use `EXPECT_CALL` for delegation contracts:
   - Good: return value, `EXPECT_CALL(age, activate(&child_gl, &copied_goal))`, `EXPECT_CALL(rules, size()).WillOnce(Return(0))`.
   - Avoid `Times(0)` on unrelated methods unless the test is explicitly about “must not call remote.”

5. **Exercise the SUT through the same surface production uses** — often `i_*&` on the concrete type.

Stack-allocated **value types** (`expr`, `rule`, lineages) are inputs, not collaborators.

### Unit test examples (core)

`core/test/unit/infrastructure/overlay_bind_map.cpp`:

- **SUT:** `overlay_bind_map`
- **Mocks:** `MockBindMap` local, `MockBindMap` remote
- **Not used:** real `bind_map`

`core/test/unit/infrastructure/goal_activator.cpp`:

- **SUT:** `goal_activator`
- **Mocks:** `MockCopier`, `MockActivateGoalExpr`, `MockGetCandidateTranslationMap`
- **Not used:** real `copier`, `expr_pool`, `lineage_pool`

`core/test/unit/infrastructure/expr_pool.cpp`:

- **SUT:** `expr_pool`
- **Mocks:** `MockTrail` (`EXPECT_CALL(trail, log(_))` for interning; `NiceMock` when trail is unused)
- **Not used:** real `trail` — backtracking survival is in `core/test/integration/expr_pool.cpp`

`core/test/unit/infrastructure/elimination_router.cpp`:

- **SUT:** `elimination_router`
- **Mocks:** `MockDeactivatedCandidateMemory`, `MockActiveGoals`, `MockEliminationBacklog`, `MockCandidateDeactivator`
- **Assertions:** `contains`, `insert`, `deactivate` via `EXPECT_CALL`, not sets on hand-written recorders

## Integration tests

An **integration test** checks behavior that only appears when **two or more real components** work together.

### Rules

1. **Use real implementations** for each type in the slice under test.

2. **Still mock types outside the slice** with GMock — same rules as unit tests, but only for dependencies that are not part of what you are integrating. Example: real `copier` + real `bind_map` + factories for unify, but `MockGetGoalExpr`, `MockCandidateActivator`, `MockEliminationBacklog` around a visitor.

3. **Assert end-state or cross-component invariants**:
   - `bind_map::whnf` after unify
   - Canonical `expr_pool` pointers after copy
   - `EXPECT_CALL(ca, activate(rl))` after a successful `visit`

4. **Do not duplicate unit coverage** — integration adds wiring unit tests cannot do with mocks alone.

### Integration test examples (core)

`core/test/integration/overlay_bind_map.cpp`:

- **Real:** `bind_map` (local), `bind_map` (remote), `overlay_bind_map`

`core/test/integration/expr_pool.cpp`:

- **Real:** `trail`, `expr_pool` — push/pop undo behavior

`core/test/integration/goal_candidate_activator_visitor.cpp`:

- **Real:** `copier`, `bind_map`, factories, `unifier`, `mhu`, `lineage_pool`, …
- **Mocks:** `MockGetGoalExpr`, `MockActivateCandidateTranslationMap`, `MockCandidateActivator`, `MockEliminationBacklog`

## Choosing unit vs integration

| Question | Unit | Integration |
|----------|------|-------------|
| Can collaborators be fully specified with `EXPECT_CALL`? | Yes | — |
| Does correctness depend on real WHNF, interning, trail undo, or bind chaining? | — | Yes |
| Is one class’s logic the focus? | Yes | — |
| Is a pipeline or factory stack the focus? | — | Yes |

When in doubt: **unit test with mocks first**; add integration when mocks would need to reimplement another component to be meaningful.

## Naming and files

- Unit: `<package>/test/unit/<area>/<component>.cpp` — `OverlayBindMapTest`, `NormalizerUnitTest`.
- Integration: `<package>/test/integration/<component>.cpp` — `*IntegrationTest`.
- Mock types: `Mock` + interface name (`MockBindMap`, `MockTrail`).

## Adding a new test

1. Decide **unit vs integration** using the table above.
2. List the SUT’s dependencies from its constructor or public API.
3. For each dependency in a **unit** test: add a `struct Mock… : public i_…` with `MOCK_METHOD`, then `EXPECT_CALL` / `ON_CALL` in each test case.
4. For **integration**: use real types for the slice; GMock everything outside the slice.
5. Build and run the package debug test binary (e.g. `./build/core_debug`).

## Related code organization

In `core`, **`i_*` interfaces** (`core/hpp/interfaces/`) are separated from **infrastructure** implementations (`core/hpp/infrastructure/`) so tests can mock collaborators without linking the whole solver. See [bootstrap.md](bootstrap.md) for runtime wiring; tests construct the SUT and mocks directly.
