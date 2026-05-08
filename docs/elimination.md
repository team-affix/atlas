# Candidate Elimination

## `active_eliminator`

The single entity responsible for removing candidates from active goals. Its `eliminate(rl)` method:

1. Calls `goal_candidates_store.eliminate(rl)` — removes the resolution lineage from its parent goal's candidate set.
2. Emits `candidate_eliminated_event{rl}`.

`candidate_eliminated_event` bridges to `goal_candidates_changed_event`, feeding the detectors.

### Callers

| Caller | Reason |
|---|---|
| `router_avoidance_unit_event_handler` | A CDCL avoidance has become unit — the last remaining resolution must be eliminated to avoid completing the conflicting set. |
| `elimination_backlog` | A candidate queued for elimination (goal was not yet active) is flushed once the goal becomes active. |

## `elimination_backlog`

Exists because **a CDCL avoidance can become unit while referencing a goal not currently on the frontier**.

This arises from the leaf-only lemma design. Consider avoidance `{A:1, B:2}`. When resolution `A:1` is taken, the avoidance simplifies to `{B:2}` — unit. This means `B:2` must be eliminated. However, goal B may not be on the frontier yet: the avoidance was learned in a prior sim where B existed, but in the current sim B may not have been derived yet.

The avoidance is semantically valid regardless — it says "B:2 must never happen" independent of when B appears.

- If B **is** currently active: `active_eliminator.eliminate(B:2)` is called immediately.
- If B **is not** active: the resolution lineage is inserted into the backlog.

When goal B eventually becomes active (via `goal_activated_event`), the backlog is checked. Any queued eliminations for B are flushed: `backlogged_elimination_freed_event` is emitted for each. A handler subscribed to that event calls `active_eliminator.eliminate()`, applying the elimination as if the avoidance had become unit at that moment.
