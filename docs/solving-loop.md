# The Solving Loop

## Overview

The solver maintains a set of **active goals** — goals currently being worked on. Each goal has a set of **candidates**: database rules that could potentially resolve it.

The loop is entirely **event-driven and priority-based**. There is no explicit iteration construct — the scheduler processes queued events in priority order.

## Lifecycle: sim_active and sim_inactive

The solve process alternates between two phases:

**`sim_active`:** The main search loop is running. Goals are being resolved, CDCL constraints applied, decisions made via MCTS. Ends when a solution or conflict is detected.

**`sim_inactive`:** Cleanup and setup of the next sim — clearing stores, learning lemmas, resetting the trail frame, activating initial goals. Persistent state (MCTS tree, learned lemmas) survives into the next `sim_active` phase.

Transitions are automatic and event-driven:
- `sim_starting_event` → handler sets `sim_active = true` in `i_sim_activity_monitor`.
- `sim_ending_event` → handler sets `sim_active = false`.

> Note: the old `solver` and `sim` structs in the codebase are legacy. The current architecture folds their responsibilities into the domain directly, with the above two phases as the governing lifecycle.

## Unit resolution vs. decided resolution

There are two ways a resolution can occur:

- **Unit resolution** — forced: the goal has exactly one candidate remaining.
- **Decided resolution** — chosen: MCTS selects a goal and candidate.

These are governed by two events with deliberately ordered priorities:

| Event | Priority | Role |
|---|---|---|
| `goal_unit_event` | `p` (higher) | Signals that a specific goal has become unit. |
| `no_more_unit_goals_event` | `< p` (lower) | Signals no unit goals exist; triggers a decision. |

## How the loop runs

`no_more_unit_goals_event` is emitted once at the start of each sim. From that point:

1. Any queued `goal_unit_event` fires first. The unit goal is resolved immediately.
2. New child goals may themselves be unit, producing more `goal_unit_event`s — which again fire before `no_more_unit_goals_event`.
3. Once no unit goals remain, `no_more_unit_goals_event` fires. Two handlers respond:
   - **Decider handler:** calls `decide()` on `i_decider`, triggering a decided resolution via MCTS.
   - **Repeater handler:** re-emits `no_more_unit_goals_event`. Because it is re-queued *after* any `goal_unit_event`s the decision may produce, units always fire first. If no units are produced, the repeater fires immediately.

The MCTS tree is **shared across all sims and restarts** — statistics accumulate over the full lifetime of the solve.

## Termination events

| Event | Phase | Meaning |
|---|---|---|
| `conflicted_event` | `sim_active` | Some active goal has zero candidates. Sim ends; CDCL learns a lemma; restart triggered. |
| `refuted_event` | `sim_inactive` | No branch of the remaining search space can yield a solution. Query is unsatisfiable. |
| `solved_event` | `sim_active` | Active goal store is empty — all goals resolved to facts. A solution has been found. |

`conflicted_event` and `refuted_event` both mean "no solution reachable from current state." The difference is purely temporal: `conflicted_event` is detected *during* a sim; `refuted_event` is detected *between* sims.
