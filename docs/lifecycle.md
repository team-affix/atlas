# Sim Lifecycle

The solve process alternates between two phases:

**`sim_active`:** The main search loop is running. Goals are being resolved, CDCL constraints applied, decisions made via MCTS. Ends when a solution or conflict is detected.

**`sim_inactive`:** Cleanup and setup of the next sim — clearing stores, learning lemmas, resetting the trail frame, activating initial goals. Persistent state (MCTS tree, learned lemmas) survives into the next `sim_active` phase.

Transitions are automatic and event-driven:
- `sim_starting_event` fires first — the sim is in the process of starting. Handlers perform setup (including the full restart sequence; see [restart.md](restart.md)).
- `sim_started_event` fires once all `sim_starting` handlers have completed — the sim is now active.
- `sim_ending_event` fires when the sim is ending. Handlers perform teardown.

> Note: the old `solver` and `sim` structs in the codebase are legacy. The current architecture folds their responsibilities into the domain directly, with the above two phases as the governing lifecycle.
