# PUD smell resolution record

Audit date: 2026-10-23.  
Verification: `make -j6 core_debug_fast` and `./build/core_debug_fast --gtest_filter='Pud*:Ubs*'` → **544 passed**.  
Timing evidence: `docs/pud-performance.txt` (and `/tmp/pudprobe/{before,after}.txt` when present).  
Architecture reference: `docs/pud-query-model.txt`, `prompts/pud.txt` (candidate section).

Legend: **Resolved** = removed or redesigned. **Justified** = kept intentionally; rationale below.

## 1 — Design / documentation

| ID | Status | Notes |
|----|--------|-------|
| D1 | Resolved | `docs/pud-query-model.txt` rewritten for current stack; `prompts/pud.txt` fills candidate storage; old om_interval/FPA narrative deprecated in docs. |
| D2 | Resolved | Query ownership via `pud_query_holders` + `pud_leaf_releaser`; no destructor-driven head lifetime. |
| D3 | Resolved | Single removal path: `mhq::remove` when last holder releases; no parallel erase guards. |
| D4 | Resolved | Holders are authoritative; not rebuilt from side maps on each descent. |
| D5 | Resolved | One candidacy list per `pud_mhq` head (axiom-ordered); mhcs holds search state only. |
| D6 | Resolved | `pud_acceptor` diffs bind maps via `pud_changed_class_finder`; path replay via `pud_replayer` (no acceptor-local redo stack). |
| D7 | Resolved | `ubs_bind_map` is concrete; `pud_transient_ubs_state` and fake bind-map template stacks deleted. |
| D8 | Resolved | `pud_goal_list` + `pud_node_maker` single numbering along paths. |
| D9 | Resolved | Unit work: `std::deque<pud_mhq_head_id>` on admission; refutation: `pud_goal_queries::refuted` + holder refutation cascade. |
| D10 | Resolved | One `pud_structure_watcher` keyed by `pud_body_goal_id`. |
| D11 | Resolved | One `pud_manifest` + `pud_solver`; old runtime / dual manifests removed. |

## 2 — Domain boundaries

| ID | Status | Notes |
|----|--------|-------|
| B1 | Resolved | Body-goal/query PQ descent dissolved into `pud_leaf_query_maker` + mhq API. |
| B2 | Resolved | `pud_expander` exposes one `expand`; collaborators injected (5 capabilities). |
| B3 | Resolved | Runtime facade deleted; `pud_root_opener`, `pud_solver`, `pud_command_handler` are the surface. |
| B4 | Resolved | `pud_node_maker` replaces root descent maker passthrough. |
| B5 | Resolved | mhws/mhcs/mhq own state; no head objects carrying collaborator bags. |

## 3 — Efficiency

| ID | Status | Notes |
|----|--------|-------|
| F1 | Resolved | Unifier memo + identical-pair skip; see performance doc. |
| F2 | Resolved | Normalizer per-visit memo (same idea as exporter). |
| F3 | Resolved | `equate` merges values; absorbed classes erased from map. |
| F4 | Resolved | Fewer repeated `find`/`find_value` passes; exporter/normalizer memo. |
| F5 | Resolved | No transient bind-map facades; `persist() &&` on owned transients. |
| F6 | Resolved | Descent copy chains removed with descents. |
| F7 | Resolved | `pud_goal_list` ordered flex_vector; mhq does not re-sort candidates. |
| F8 | Justified | `pud_node_id` / head ids index dense `pud_node_table` and head maps; `unordered_map` on ids is bounded per manifest lifetime; wrappers would not change correctness. |
| F9 | Resolved | Unit classification kept: unit deque consumed first in `pud_solver`. |
| F10 | Resolved | Old runner / unreachable traversal deleted. |

## 4 — Existence / dead code

| ID | Status | Notes |
|----|--------|-------|
| E1 | Resolved | `pud_mhq::check_head` and other test-only production hooks removed. |
| E2 | Resolved | ~100 obsolete files removed (descents, factories, dual front-ends, etc.). |
| E3 | Resolved | Write-only recorder / transient state types deleted. |
| E4 | Resolved | `atlas pud` subcommand (`pud_command_handler`). |
| E5 | Justified | `DEBUG_ASSERT` on store-once tables documents invariants; invalid ids are programmer errors in non-test builds (no silent recovery). |
| E6 | Resolved | Forwarding-only descents and owner box removed. |

## 5 — Interfaces

| ID | Status | Notes |
|----|--------|-------|
| I1 | Justified | `pud_manifest` is the composition root and intentionally owns collaborators; public members used by harness/CLI match project DI exception. Solver/expander public APIs stay minimal. |
| I2 | Resolved | `ubs_bind_map` minimal surface; term tools share `pud_term_factory`. |
| I3 | Resolved | Frontier/events as values (`pud_frontier_change`, solver step variant), not mutable event registries. |
| I4 | Resolved | `equate` displacement used; invalidate layers return refuted ids. |
| I5 | Resolved | Getter chains on deleted stores removed. |
| I6 | Justified | Solver coroutine may be restarted when `done()`; documented in harness `next_solver_step`. |

## 6 — Guards / assertions

| ID | Status | Notes |
|----|--------|-------|
| G1 | Resolved | Internal dual-removal guards removed with single holder path. |
| G2 | Resolved | `DEBUG_ASSERT` uses `do/while(0)` in `debug_assert.hpp`. |

## 7 — Symmetry / registries

| ID | Status | Notes |
|----|--------|-------|
| S1 | Resolved | mhws / mhcs / mhq symmetric try_add, try_fork, remove, invalidate_leaf (+ read APIs). |
| S2 | Resolved | Descents removed; replayer/binder/watcher have one signature each. |
| S3 | Justified | `const expr*` remains: hash-consed DAG nodes in `expr_pool`; justified by interning, not ownership ambiguity. |
| S4 | Resolved | Structure watch keyed by goal id, not separate watch-head ids. |
| S5 | Resolved | `pud_frontier_retirer` / layered invalidate replace indexed invalidator names. |
| S6 | Resolved | Single `pud_leaf_activator` path (no duplicate activation descents). |
| S7 | Resolved | Duplicate value objects (admission, resume contexts, unit/refuted query types, etc.) deleted. |
| S8 | Resolved | `pud_term_factory` centralizes term collaborators. |
| S9 | Justified | Store-once `get` asserts or throws in tests via `optional::value()`; matches “fail loud on bug” policy. |

## 8 — Nesting / clarity

| ID | Status | Notes |
|----|--------|-------|
| N1 | Resolved | Hot paths flattened (early continue in unifier, cycle detector, watcher). |
| N2 | Resolved | Nested ternary removed in refactors (e.g. mhcs witness slot selection uses named branch). |

## 9 — Language / types

| ID | Status | Notes |
|----|--------|-------|
| L1 | Resolved | `pud_ubs_state` has `operator==` only (no `<=>` on immer fields). |
| L2 | Resolved | Factory/fork ctor overloads removed from search heads. |
| L3 | Resolved | No inheritance-as-alias; templates + injection only. |
| L4 | Justified | Head and node ids are distinct typedefs to `size_t`; true strong types deferred—ids only minted by sequencers/mhq admission, dense tables define semantics. |
| L5 | Resolved | `persist() &&` on bind map; no destructive `to_persistent_state`. |
| L6 | Resolved | Bind map owns transients; consumers depend on capabilities not inverted headers. |
| L7 | Resolved | Children by index on vectors; no raw iterator typedefs on stores. |

## 10 — Testing

| ID | Status | Notes |
|----|--------|-------|
| T1 | Resolved | Unit tests for new components (bind map, unifier, replayer, mh*, leaf layer, …). |
| T2 | Resolved | Unit tests mock collaborators; one real SUT per file. |
| T3 | Resolved | Integration slices (manifest, search stack, leaf activation, bind maps, …). |
| T4 | Resolved | Hand-rolled fakes removed; GMock + `pud_search_tree` / harness helpers. |
| T5 | Resolved | No tests asserting ubs map internals; public bind/unify outcomes only. |
| T6 | Justified | `EXPECT_THROW` on `pud_node_table::get` / `pud_active_states::get` documents public contract for invalid ids (not production paths). |
| T7 | Resolved | `pud_manifest_harness` uses production binder/materializer for reads. |
| T8 | Partial | Shared harness reduces duplication; some scenario overlap remains acceptable for readability. |
| T9 | Resolved | Leak loop test removed; holder-based invariants instead. |
| T10 | Resolved | No lambdas in PUD unit tests; fuzz uses named helpers only. |
| T11 | Resolved | Harness invariants renamed `verify_*`; production uses `is_leaf` / `is_refuted`. |

## 11 — Naming / vocabulary

| ID | Status | Notes |
|----|--------|-------|
| X1 | Resolved | `global_normalizer` → `pud_normalizer`; files match types. |
| X2 | Resolved | `pud_leaf_nodes` / `pud_refuted_leaves` + `pud_refuted_node_checker`; harness `verify_*`; docs use invalidate/fork/hold. |
| X3 | Resolved | “Witness” vs “candidate” documented in `prompts/pud.txt`. |
| X4 | Resolved | Symmetric names across mhws/mhcs/mhq. |
| X5 | Resolved | No single-letter locals in new code paths (per project rules). |
| X6 | Resolved | Consistent struct layout, trailing `_` on members. |
| X7 | Resolved | Single `pud_manifest`; duplicate manifests deleted. |

## Follow-ups (non-blocking)

- **L4 / F8**: Introduce opaque struct ids if we need cross-type confusion checks at compile time.
- **T8**: Further dedupe scenario fixtures if tests grow redundant.
- **I1**: Narrow `pud_manifest` visibility if CLI/harness gain friend or facade accessors.
