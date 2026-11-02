# Atlas — project memory for Claude Code

## What this is

Atlas is a **pure relational CHC (Constrained Horn Clause) solver** in C++20. A `.chc` rule
database defines the problem; the query is passed on the command line (`--goal "p(X), q(X)"`).
It prints `SOLVED` + bindings for each distinct model and `REFUTED` when it runs out.
It is pure Horn: no theories, no arithmetic built-ins. Rule order and body-atom order carry no meaning.
Its design goals are model completeness and fair search.

Two generations of engine live side by side:

- **Sim-based solvers** (`horizon`, `ridge`, `genius`, `quell`, `phoenix`, `basic`, plus
  `dbuct_*` and `_fc` / `_bt` / `_fgt` variants). These run forward episodes with CDCL learning,
  MHU and MCTS. Each one is a `<name>_manifest` + `<name>_runtime` in `core/` and a
  `<name>_command_handler` in `cli/`. See `README.md` and `docs/solving-loop.md`, `docs/solvers.md`.
- **PUD (progressive unfolding database)** is the **current focus**. Modules are append-only
  resolution trees that serialize to binary (`atlas module create|inspect`). A live frontier of leaves
  sits on top. **Lodestar** (`pud_lodestar_*`, `atlas lodestar`) is the first automated PUD solver.
  Read `docs/pud-architecture.md` and `docs/pud-solver-architecture.md` before touching `pud_*`.

`README.md` predates PUD and lists only some of the subcommands. `cli/entry/main.cpp` is the
source of truth for the CLI.

## Layout

```
core/hpp/{infrastructure,value_objects}   headers   (behavior types | plain data)
core/cpp/{infrastructure,value_objects}   .cpp for non-templated types
core/test/{unit,integration}, main.cpp    GTest/GMock
parser/                                    ANTLR 4.10.1 grammar + .chc front-end
cli/{entry,hpp,cpp,test,examples}          CLI11 front-end; examples/*/db.chc
mcts/ CLI11/ googletest/ immer/            git submodules, not ours to edit
debug_assert.hpp                           DEBUG_ASSERT(cond): throws std::logic_error under -DDEBUG
docs/                                      design notes (see docs/continuous-documentation.md)
```

## Architecture

### Layering (same for every solver, old and PUD)

```
cli/entry/main.cpp            CLI11 subcommand → options struct → handler(...)()
  <name>_command_handler      cli/: loads input, owns the runtime, printers, timers, progress loop
    <name>_runtime            core/: non-templated facade (.hpp/.cpp). Small API like next(),
                              solution(), solved(). Owns exactly one manifest_.
      <name>_manifest         composition root: names the component types, owns every
                              component by value, wires them through constructor refs
        components            small templated infra types; each depends only on the capabilities it calls
```

- The runtime is the only thing callers see. Templates are instantiated once, inside the manifest/runtime `.cpp`.
- Each runtime exposes a pull loop: callers call `next()` until it returns false, reading
  `solution()` / bindings after each one.
- When you add a solver, add a new manifest + runtime + command handler + `main.cpp` subcommand
  instead of changing an existing one. Duplicate wiring across manifests if needed.
- Solver policy types take capabilities like everything else. `docs/pud-solver-architecture.md`
  (decision 3) still allows a heuristic to read its own manifest's internals, but Lodestar no longer needs to:
  its driver is a small orchestrator over injected parts (query lister, ring builder, round-robins,
  unit propagator, unfold applier).

### Sim-based engine (`docs/solving-loop.md`, `docs/lifecycle.md`)

The `solver` loop is setup → `run_sim` (one forward episode, no backtracking) → CDCL learn → teardown.
State kept across sims: CDCL avoidances, MCTS tree, pinned lineages (`lineage_pool`), db, initial goals.
Per-sim state (goals, candidates, bind map, expr pool frames, MHU heads) is frame-pushed and undone.
Solvers differ only in decision policy and MCTS reward, which come from what the manifest injects.

### PUD engine (`docs/pud-architecture.md`)

- **Stored plane:** `pud_modules` → `pud_module` trees (node goals, axioms, appended inferences).
  Nodes are never deleted, and refutation is a flag. This plane is what gets serialized.
- **Live plane:** frontier leaves with `pud_active_state` (binding snapshot, structure watch, body goal → MHQ head),
  plus `pud_query_holders`. This plane is rebuilt by activation on load and never persisted.
- **Search stack per query head:** `pud_mhq` → `pud_mhcs` → `pud_mhws` → `pud_replayer`, which walks stored inferences.
- `unfold(leaf, body_goal)` materializes a child via resolver/acceptor, moves activity to the
  children (`pud_activity_mover`) and retires the parent. Lodestar's driver decides which unfolds to make.
- A live leaf holds a query for every goal: a failed open or fork refutes the node. Code asserts this
  (`DEBUG_ASSERT`) rather than guarding the impossible case.
- `pud_runtime` / `pud_manifest` are the manual harness tests drive; they live in `core/test/`.
- Lodestar sessions end with a `pud_search_stop_reason`: `refuted`, `exhausted` (node limits block every
  unfold), or `budget_spent` (`--max-unfolds` counts every unfold). See `docs/pud-solver-architecture.md`.

## Build & test

- Iterate with the `_fast` targets (`-DDEBUG -O3`): `make core_debug_fast && ./build/core_debug_fast`
  (same for `parser_` and `cli_`). Use `--gtest_filter=...` to narrow runs.
- Release binary: `make atlas` → `./build/atlas <subcommand> ...`
- The makefile `find`s every `.cpp` under `core/cpp`, `core/test`, `cli/...` and so on.
  **A new file is picked up automatically, so don't edit the makefile to register it.**
- CI (`.github/workflows/release.yml`) builds and runs all three `_debug_fast` binaries.

## Code style — the rules

The detailed rules live in `.claude/rules/*.md` and load automatically every session. They cover
philosophy, naming, type design, dependency injection, headers/templates, no-lambdas,
simplification, and testing (policy, unit vs integration, fixtures, mocks). Edit those files to change a
rule, not this summary.

Core of it in one breath: simplest thing that works. `struct` only. Collaborators are injected **by
reference as template params named per invoked capability** (`IFindSolvedLeaf`, not `IFinder`).
The only other inheritance besides the ANTLR visitors is the manifest naming trick (see below).
Manifests are the composition root and own their parts by value. No lambdas or `std::function`,
no overloads, no default args, no default member initializers. Paired operations are mirror images.

## Conventions observed in the code (not spelled out in the rules)

- **Out-of-line template definitions** in newer headers use a macro pair that is `#undef`'d at the end of the header:
  ```cpp
  #define PUD_X_TEMPLATE template<typename IA, typename IB>
  #define PUD_X          pud_x<IA, IB>
  PUD_X_TEMPLATE bool PUD_X::take() { ... }
  #undef PUD_X
  #undef PUD_X_TEMPLATE
  ```
- **Manifests name their component types** as nested structs that inherit the instantiation:
  `struct mhq_t : pud_mhq<...> { using pud_mhq<...>::pud_mhq; };`. This keeps later template
  argument lists short and allows forward declaration for cycles. Members are declared and initialized
  in dependency order, and one object fills several capability slots (`modules_, modules_, modules_`).
- **Constructor init lists** put the comma first:
  ```cpp
  : solution_(nullptr)
  , solution_depth_(0)
  ```
- **Undo/backtracking**: a stateful type keeps `std::stack<frame>` of `std::list<X_action>`. `X_action`
  is a `std::variant` of small value objects named `<thing>_insert` / `_erase` / `_assign`. Each type
  has private `log(action)` and `undo_action(action)` plus public `push_frame()` / `pop_frame()`.
- **Value objects** use `auto operator<=>(const T&) const = default;`. Tests build them with designated
  initializers (`pud_global_node_id{.module = 7, .node = 4}`). Ids are their own value-object headers
  (`pud_module_id`, `pud_node_id`, …).
- `std::optional` is checked with `.has_value()` and read with `.value()`, not `*opt`. Locals are `const` wherever possible.
- `DEBUG_ASSERT` guards preconditions in `.cpp`/template bodies. Tests run with it enabled.
- Keep lines to about 120 columns.
- **Tests**: mocks go in an anonymous `namespace {}` at the top of the file along with the fixture
  `struct <Component>Test : ::testing::Test`. The SUT is a fixture member built with brace-init.
  Use `InSequence` when call order is the contract. Test names are behavior sentences in CamelCase
  (`ASolvedLeafIsNormalizedStoredAndErased`).
- Domain prefixes: `pud_` (progressive unfolding database, including generic solver plumbing over it),
  `pud_lodestar_` (only Lodestar's own heuristic and composition: its driver, rings, slots, manifest, runtime),
  `mhq` (multi-headed query; `mhcs` picks candidates, `mhws` searches witnesses, per
  `docs/pud-architecture.md`), `ubs` (union-by-size bind map), `mhu` (multi-head unification),
  `dbuct_` (a solver family that parallels the plain one).

## PUD-specific decisions (from docs/pud-solver-architecture.md)

- Only `pud_module` is persisted, and it holds no solver-specific fields. Live frontier and solver state are rebuilt on load.
- **Do not grow `pud_runtime` for a solver.** Each solver gets its own `pud_<name>_manifest`.
  Duplicating kernel wiring across manifests is acceptable. Heuristics may read their own manifest's internals.
- When a solver needs variant behavior, prefer `pud_<solver>_*` forwarding types. A small read-only query on a
  kernel type is fine when re-deriving the answer outside it would be wrong (e.g. `pud_mhws::find_witness`).
- A recent direction: break big manifests into small single-purpose infra types (`*_taker`, `*_finder`,
  `*_store`, `*_seeder`, `*_stepper`), each with its own unit test.

## Working agreements

- **Don't create files unless asked.** That includes docs, reports and scratch notes in the repo; use the scratchpad instead.
- When asked only to write tests, **don't touch non-test source**. If a test exposes a production bug,
  leave it failing and report it. Fix it only if told to. Never weaken or delete a test to make it pass.
- New public behavior needs a unit test (one real SUT, everything else mocked). Add an integration test only
  when correctness depends on real components interacting.
- Docs: follow `docs/continuous-documentation.md`. Write down only what was stated, and ask instead of guessing.
- Be terse. No superlatives, get to the point.
- Git: commit messages are short, lowercase, past tense ("added unit propagation to lodestar").
  Branches use `feature/`, `enhancement/`, `bug-fix/` prefixes. Default branch is `master`.

## Stale / don't trust blindly

- `aipromptmemory.md` predates the GTest migration. Its `TEST()` macro / `./main` / `assert()` guidance is
  obsolete. Its working agreements are carried over above.
- `README.md` solver list is incomplete (see above). `test_audit.md` is a one-off Quell audit.
