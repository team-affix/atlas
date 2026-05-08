# Sim Restart Sequence

The restart sequence takes place during the `sim_starting` phase. It is split across two methods on `sim_restarter`, separated by an event-driven gap.

## `begin_restart()`

1. **`trail.pop()`** — pops the sim's trail frame, undoing all variable bindings, goal store entries, and `expr_pool` allocations from the finished sim.
2. **`decision_store.derive_lemma()`** — derives a leaf-only decision lemma from the set of decisions made during the sim. Stored as `pending_lemma`.
3. **Emits `goal_stores_clearing_event`** — signals all 7 store-clearing handlers to call `clear()` on their respective stores:
   - `goal_expr_store`
   - `goal_candidates_store`
   - `goal_weight_store`
   - `active_goal_store`
   - `inactive_goal_store`
   - `resolution_store`
   - `decision_store`

The event chain fires: `goal_stores_clearing_event` → (all clearing handlers run) → `goal_stores_cleared_event`.

`goal_stores_cleared_event` has a lower priority than `goal_stores_clearing_event`, guaranteeing all clearing handlers complete before it fires.

A handler for `goal_stores_cleared_event` calls `sim_restarter::complete_restart()`.

## `complete_restart()`

4. **`c.learn(pending_lemma)`** — gives the decision lemma to CDCL, creating a new avoidance that prevents the same decision set from being taken again.
5. **`pending_lemma.reset()`** — clears the inter-phase state.
6. **`trail.push()`** — opens a fresh sim frame on the trail.
7. **`initial_goal_activator.activate_initial_goals()`** — re-activates the initial goals, emitting `initial_goal_activating_event` for each, which triggers the initializers to repopulate all stores with fresh candidates, expressions, and weights.

At this point the system is fully set up for the next sim: stores populated, trail frame open, CDCL updated. Once all `sim_starting` handlers have completed, `sim_started_event` fires — driving the new sim. Since initial goals were activated during `sim_starting`, `goal_unit_event`s for those goals may already be queued by the time `sim_started_event` fires.
