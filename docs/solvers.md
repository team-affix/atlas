# Solvers

## Horizon

Uses **CGW (Cumulative Grounded Weight)** as the MCTS reward — a value in `[0, 1]` representing how much of the proof tree has been grounded against facts in the current sim. Higher is better.

See [resolution.md § Goal weights and CGW](resolution.md#goal-weights-and-cgw) for how CGW is computed.

## Ridge

Certainty-based search. Has no notion of "closeness to a solution." Treats all remaining candidate paths fairly and focuses on finding and eliminating **shallow conflicts** first.

**Reward:** `-(number of decisions made so far in this sim)` — the total number of resolutions chosen, not just the leaf count. This is deliberately **not** the negative of the decision *lemma* size (which counts only leaves).

**Reasoning:** A conflict discovered after many decisions is deep in the search tree. The learned lemma, while valid, covers a very narrow slice of the space and will rarely fire again. A conflict discovered after *few* decisions (small decision set, reward closer to zero) generalises broadly and prunes a large portion of the search space. Ridge therefore steers MCTS toward decisions that tend to produce early, high-value conflicts.

## Quell

MCTS-guided search that rewards **minimizing remaining work** rather than raw active-goal count.

**Reward:** `-remaining_work`, where

```
remaining_work = Σ_{active goals g} f(depth(g))
f(l) = 1 + exp(-K * (l - J))
```

- `l` is the goal’s lineage depth (initial goals are depth `0`; each child is parent depth `+ 1`).
- `K` and `J` are configurable hyperparameters (`--work-decay-k`, `--work-decay-j`; defaults `0.2` and `10`).
- At midpoint depth `J`, `f(J) = 2`. Shallow goals contribute more work; deep goals approach `1` from above.

**Reasoning:** Minimizing only the number of active goals encourages early termination (few initial goals, little work done). Weighting each goal by `f(depth)` approximates work still left in the proof, so sims that expand shallow obligations are preferred over sims that stall near the root.

**Variants:** `quell`, `quell-fc`, `dbuct-quell`, `dbuct-quell-fc`.

## Genius *(ridge + horizon)*

Dual reward: horizon CGW at rule-scope nodes, ridge decision-count otherwise. See the genius manifests.

## Ridge-Horizon *(planned)*

Combines the strengths of both: certainty-based pruning from `ridge` with the progress-aware heuristic of `horizon`.
