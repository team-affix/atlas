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

## Drain events

A **drain event** is an event that is assigned an extremely low priority so that it fires only after the events that matter have been processed.

The pattern involves a **leading event** and a **trailing (drain) event**:

- The **leading event** fires and triggers a burst of activity — many handlers run, new events get queued, side effects accumulate.
- The **drain event** sits at the back of the queue with the lowest priority. It fires only once everything triggered by the leading event has been fully processed.

The key property is that the drain event's priority is set lower than everything it must follow, so by the time it fires, the relevant preceding work has completed.

### Example: `sim_stopping_event` → `sim_stopped_event`

`sim_stopping_event` is the leading event. Among other things, it bridges to `sim_cancelled_event`, which causes all cancellable event handlers to discard any pending work still in the queue.

`sim_stopped_event` is the drain event. It is given a priority low enough that it processes only after all events triggered by `sim_stopping_event` — including the cancellation flush — have finished. By the time `sim_stopped_event` fires, the cancellation flush has completed.

### Example: `goal_unit_event` → `no_more_unit_goals_event`

`goal_unit_event` is the leading event — it fires whenever a goal becomes unit and immediately triggers a resolution. That resolution may produce new child goals, some of which may also be unit, queuing further `goal_unit_event`s.

`no_more_unit_goals_event` is the drain event. Its lower priority means it sits behind all pending `goal_unit_event`s. It only fires once no unit goals remain in the queue — at which point it triggers a decision via MCTS.

Note: the drain pattern does not require cancellation. Even if the events triggered by the leading event are actively processed (rather than discarded), the trailing drain event is still useful as a guaranteed "everything above me has run" signal — as long as its priority is set lower than everything it must follow.

## Cancellable event handlers

Most application-layer event handlers derive from `cancellable_event_handler<Event, sim_cancelled_event, sim_cancellation_reset_event>`. This allows a handler's `execute()` to be halted if `sim_cancelled_event` is received, and re-enabled when `sim_cancellation_reset_event` fires.

Store-clearing handlers (responding to `goal_stores_clearing_event`) derive from plain `event_handler` since they should always run regardless of cancellation state.
