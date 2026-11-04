# PUD architecture

Progressive unfolding database (PUD): each registered **module** is an append-only resolution tree (axioms, then stored inferences). A **frontier** of **active leaves** sits on top: bindings, structure watches, and open **multi-headed queries** (MHQ). The caller expands by `unfold(leaf, body_goal)`; the runtime picks candidates, materializes child nodes, moves activity to children, and retires the parent. `pud_child_shuffler` stores an unfold's children in a pseudo-random order seeded by the parent's id, so every depth-first walk over children (witness searches included) is independent of rule order.

Deeper query semantics: [pud-query-model.txt](pud-query-model.txt). Design history: [pud-smell-resolution.md](pud-smell-resolution.md). Solver and driver composition: [pud-solver-architecture.md](pud-solver-architecture.md).

---

## Two planes of state

PUD keeps **stored structure** separate from **what is live on the frontier**. Search machinery walks the stored tree but does not duplicate it as a second query tree.

```mermaid
flowchart LR
  subgraph stored [Stored per module]
    T[pud_module tree]
    NG[node goals + axioms]
  end
  subgraph live [Frontier]
    AS[active field tables]
    QI[pud_query_introducers]
  end
  subgraph search [Per query head]
    MHQ[mhq]
    MHCS[mhcs]
    MHWS[mhws]
    RP[replayer]
  end
  T --> NG
  AS --> MHQ
  MHQ --> MHCS --> MHWS --> RP
  RP --> T
  unfold --> live
  unfold --> stored
```

**Stored:** `pud_module_registry` registers `pud_module` instances. Each node has its body goals and its root axiom (both `pud_node_table`s, filled on activation), and inferences are appended when unfold succeeds. Nodes are never deleted; refutation is a flag on the tree.

**Live:** Only leaves (and the transient parent during an unfold step) have active fields, each in its own `pud_node_table`: the local variable count, a binding snapshot (`pud_ubs_state`), a structure watch, and the node's queries (`pud_node_queries`, a map from `pud_body_goal_id` to a boxed `pud_mhq_head_owner`). The root and child activators store them; the node deactivator erases them when the node is retired. Fields that outlive activation (body goals, root axiom, and in Lodestar the node depth) stay. Each reader injects only the field tables it reads.

**Query inheritance is structural.** A child copies its parent's query map and changes only the goals it forks, closes or adds, so it shares the head owners of every other goal. A head is removed (`pud_query_remover`) when the last live node's map stops referencing its owner. `pud_query_introducers` records only the node that opened or forked each query and the goal it answers there. `pud_query_leaf_finder` walks down from that node to the live leaves that still hold the query, skipping refuted subtrees and children that closed the goal; unit propagation and stranded refutation read the leaves from it.

**Search:** For one body goal’s query head, `pud_mhq` coordinates heads; `pud_mhcs` chooses among axiom-level candidates; `pud_mhws` DFS-es witness paths along one edge; `pud_replayer` walks the stored inference chain inside a single `ubs_bind_map`, unifying substitutions. Resolver and inference builder use the same stack when materializing a chosen candidate into a `pud_inference`.

---

## Composition and entry

Callers own `expr_pool` and module payloads. Each solver has its own manifest, the composition root that wires the kernel by template injection, and a runtime facade over it: Lodestar's are `pud_lodestar_manifest` and `pud_lodestar_runtime`. The diagrams below use `pud_runtime` / `pud_manifest`, the manual harness that tests drive; it lives in `core/test/`.

```mermaid
flowchart TB
  Caller[Caller: expr_pool + modules]
  RT[pud_runtime]
  MF[pud_manifest]

  Caller --> RT
  RT --> MF

  MF --> Mod[pud_module_registry]
  MF --> MA[module_activator]
  MF --> UF[unfolder]
  UF --> Res[resolver]
  Res --> Acc[inference_builder]
  Res --> MHQ
  MF --> CA[child_activator]
  MF --> RA[root_activator]
  CA --> Bind[binder + structure_watcher]
  RA --> MHQ[mhq stack]
  Acc --> Terms[bind_map / unifier / term_exporter factories]
  RP[replayer] --> Terms
```

**Registration path:** `register_module` → `pud_module_registry` stores the tree → `pud_module_activator` activates axiom roots (`pud_root_activator`) and walks interior nodes like a batch of mini-unfolds (child activation, refute dead ends, deactivate parents). Result: `pud_frontier_change` (new active leaves).

**Unfold path:** `unfolder` calls `resolver` for the leaf’s goal, stores each `pud_inference` on the module, activates every child while the parent still holds queries (`pud_child_activator`), refutes children that cannot get candidates, deactivates the parent (`pud_node_deactivator`), and refutes leaves stranded without candidates (`pud_leaf_refuter` + `pud_refutation_propagator`). Same frontier-change shape as register.

Public handles are `pud_global_node_id` (module + local `pud_node_id`) and `pud_body_goal_id` (stable slot on a node’s goal list—not an expression pointer).

---

## Register module (sequence)

```mermaid
sequenceDiagram
  participant C as Caller
  participant RT as pud_runtime
  participant M as pud_module_registry
  participant MA as module_activator
  participant RA as root_activator

  C->>RT: register_module
  RT->>M: store module + edges
  RT->>MA: activate_module
  loop each viable axiom root
    MA->>RA: bindings, watch, open MHQ per goal
  end
  loop descend stored children
    MA->>MA: activate / refute / deactivate like unfold
  end
  MA-->>C: frontier_change
```

---

## Unfold one goal (sequence)

```mermaid
sequenceDiagram
  participant UF as unfolder
  participant R as resolver
  participant A as inference_builder
  participant M as pud_module
  participant CA as child_activator
  participant LD as leaf_refuter / deactivator

  UF->>R: resolve(leaf, goal)
  loop each MHCS candidate
    R->>A: unify + replay + export body
    A-->>R: pud_inference
    R-->>UF: inferences
  end
  loop each inference
    UF->>M: store child node
    UF->>CA: activate while parent still active
  end
  UF->>LD: refute failed children, deactivate parent, cascade
  UF-->>Caller: frontier_change
```

The resolver reads the leaf’s queries, bindings, variable count and goal expression, iterates candidates from the goal’s MHQ head, and delegates each to the **inference builder**: build bind map from caller bindings, unify call site with candidate head, replay the witness path, export callee body goals into the caller frame, and produce substitutions for the child binder. **Child activator** merges parent `pud_ubs_state` with those substitutions, installs structure watch, inherits/forks/opens queries, and registers holds.

---

## Term environment (one bind map per step)

Unification, normalization, export, and binding all share the same pattern: `pud_ubs_state` → `make_bind_map` → `ubs_bind_map`, then framed terms (`framed_expr` / `framed_functor`) with `globalizer` turning frame-local indices into class keys.

```mermaid
flowchart TB
  S[pud_ubs_state]
  BM[ubs_bind_map]
  S -->|bind_map_factory| BM
  BM --> U[unifier + cycle_detector]
  BM --> N[normalizer]
  BM --> E[term_exporter]
  U --> Acc[inference_builder / replayer]
  N --> Acc
  E --> Acc
  BM --> B[binder on activate]
```

Small **factories** (`pud_bind_map_factory`, `pud_cycle_detector_factory`, `pud_unifier_factory`, `pud_normalizer_factory`, `pud_term_exporter_factory`) each expose one `make_*` so tests can mock a single capability. Accept/replay paths edit the map and record which caller classes were unbound when touched; exporter rebuilds goal expressions in the caller’s LVC with a translation map for fresh variables.

Functor ids are module-local in stored terms. `pud_symbol_table` interns global functor names, and `pud_link_table` maps each module's local ids to global ids. `pud_functor_linker` fills both when a module is registered. `pud_unifier` compares head functors by their global ids, `pud_demoduler` rewrites a module-local term into global ids (used by the cycle detector and the normalizer), and `pud_functor_relinker` maps a functor into another module, appending its name and link when that module lacks it (used by the term exporter).

`pud_structure_watcher` (with `expr_pool` for functor shape) tracks which body goals changed under the child substitution so MHQ can fork or reopen heads without rescanning the whole goal list blindly.

---

## Where code lives

Templated infrastructure lives in headers under `core/hpp/infrastructure/pud_*.hpp`; non-templated types have their definitions in `core/cpp/infrastructure/pud_*.cpp`. Shared shapes and ids live in `core/hpp/value_objects/pud_*.hpp`. `pud_lodestar_manifest.hpp` (or `core/test/pud_manifest.hpp` for the manual harness) is the map of how types connect; start there, then follow the unfold chain (`unfolder` → `resolver` → `inference_builder`) or the activation chain (`root_activator` / `child_activator` → `binder`, `structure_watcher`, `mhq`). Integration coverage is `core/test/integration/pud_*.cpp`; unit tests mock collaborators in `core/test/unit/infrastructure/pud_*.cpp`. `pud_term_stack.hpp` bundles real term factories for tests.

---

## Mental model (ASCII)

```text
  STORED TREE                    FRONTIER                         SEARCH
  axioms + inferences     →      bindings + watch + MHQ     →     MHQ → MHCS → MHWS → replayer
  (immutable append)             (transient per leaf)              (finds axiom paths in tree)
```

Replay state and query heads attach to the **same** nodes as the resolvent tree; there is no parallel query DAG. Deprecated ideas are listed at the end of [pud-query-model.txt](pud-query-model.txt).
