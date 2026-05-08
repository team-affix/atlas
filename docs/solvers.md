# Solvers

## Horizon

Uses **CGW (Cumulative Grounded Weight)** as the MCTS reward — a value in `[0, 1]` representing how much of the proof tree has been grounded against facts in the current sim. Higher is better.

See [resolution.md § Goal weights and CGW](resolution.md#goal-weights-and-cgw) for how CGW is computed.

## Ridge

Certainty-based search. Has no notion of "closeness to a solution." Treats all remaining candidate paths fairly and focuses on finding and eliminating **shallow conflicts** first.

**Reward:** `-(number of decisions made so far in this sim)` — the total number of resolutions chosen, not just the leaf count. This is deliberately **not** the negative of the decision *lemma* size (which counts only leaves).

**Reasoning:** A conflict discovered after many decisions is deep in the search tree. The learned lemma, while valid, covers a very narrow slice of the space and will rarely fire again. A conflict discovered after *few* decisions (small decision set, reward closer to zero) generalises broadly and prunes a large portion of the search space. Ridge therefore steers MCTS toward decisions that tend to produce early, high-value conflicts.

## Ridge-Horizon *(planned)*

Combines the strengths of both: certainty-based pruning from `ridge` with the progress-aware heuristic of `horizon`.
