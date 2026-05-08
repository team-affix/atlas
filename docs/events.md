# Event System

## Naming convention

The tense of an event name is semantically significant:

- **Present participle (`*ing`)** — e.g. `sim_starting_event`, `goal_activating_event`: the action is **in progress**. Listeners should treat the subject as not yet fully transitioned.
- **Past tense (`*ed`)** — e.g. `goal_activated_event`, `goal_resolved_event`: the action is **complete**. All handlers for the preceding `-ing` event have already run.

If you need to react *after* something is done, subscribe to the past-tense event.

## Resolution event chain

When a resolution is chosen (unit or decided), `goal_resolver` emits a precise sequence of events in priority order:

```
goal_resolving_event         ← resolution beginning; stores not yet consistent
goal_activating_event        ← emitted for EACH child goal
goal_activated_event         ← (bridge) emitted per child; all stores have this child in valid state
goal_deactivating_event      ← parent goal being removed
goal_deactivated_event       ← (bridge) parent goal fully removed from all stores
goal_resolved_event          ← entire resolution complete; all stores consistent
```

| Event | Meaning for listeners |
|---|---|
| `goal_activating_event` | Child goal being inserted; handlers call `insert()` on each store. Goal not yet fully registered. |
| `goal_activated_event` | All stores have the child goal as a valid entry. Safe to act on it. |
| `goal_deactivating_event` | Parent goal being removed; handlers call `erase()` / move-to-inactive. Not yet gone. |
| `goal_deactivated_event` | Parent goal fully removed from all stores. |
| `goal_resolving_event` | Resolution beginning. No store is in a consistent state. |
| `goal_resolved_event` | Entire resolution complete — children active, parent inactive, all stores consistent. |

## Full priority ordering

All resolution events have higher priority than `goal_unit_event`, which has higher priority than `no_more_unit_goals_event`. A full resolution completes atomically before the loop considers new unit goals or makes decisions.

```
goal_resolving_event
goal_activating_event
goal_activated_event
goal_deactivating_event
goal_deactivated_event
goal_resolved_event
─────────────────────
goal_unit_event
─────────────────────
no_more_unit_goals_event
```

## `goal_candidates_changed_event`

A **derived event** — never emitted directly. Bridged from:

| Source | Reason |
|---|---|
| `goal_activated_event` | Activating a goal gives it an initial candidate set. |
| `candidate_eliminated_event` | A candidate has been removed from a goal's set. |

This keeps detectors decoupled from the specific causes of candidate set changes.

## Detectors

Both subscribe to `goal_candidates_changed_event`.

### `goal_unit_detector`

Checks if the affected goal now has exactly one candidate. If so, emits `goal_unit_event`.

### `goal_candidates_empty_detector`

Checks if the affected goal now has zero candidates. If so, emits `conflicted_event`.

Both run at a priority below the resolution chain but above `goal_unit_event` and `no_more_unit_goals_event`.

## Cancellable event handlers

Most application-layer event handlers derive from `cancellable_event_handler<Event, sim_cancelled_event, sim_cancellation_reset_event>`. This allows a handler's `execute()` to be halted if `sim_cancelled_event` is received, and re-enabled when `sim_cancellation_reset_event` fires.

Store-clearing handlers (responding to `goal_stores_clearing_event`) derive from plain `event_handler` since they should always run regardless of cancellation state.
