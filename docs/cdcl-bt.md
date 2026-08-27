# Binary-tree CDCL (`ridge-bt`, `dbuct-ridge-bt`)

`ridge-bt` is the ridge solver with a separate CDCL store, `bt_cdcl_elimination_generator`. Existing CDCL (`cdcl_elimination_generator`) is unchanged.

`dbuct-ridge-bt` is the delayed-backtracking ridge solver with `dbuct_bt_cdcl_elimination_generator`. Existing dbuct CDCL (`dbuct_cdcl_elimination_generator`) is unchanged.

## Store

Learned avoidances are interned as a binary tree of factors. Each factor is a tuple of resolution lineages. Split is a function of the sorted tuple:

- size 1: a leaf
- even size: two equal halves
- odd size: the last member is joined at the top

Identical tuples share interned nodes.

Each factor stores how many of its members have been visited this sim (`constrain`). When that count equals the tuple size, the factor notifies its parents. `chosen_goal_candidates` is not supplied to the restarting `bt_cdcl_elimination_generator`.

A NAND terminal whose remaining unvisited members equal 1 yields that remaining resolution.

`cleanup()` on `bt_cdcl_elimination_generator` advances a generation so visit counts do not carry into the next sim.

## Delayed backtracking (`dbuct-ridge-bt`)

`dbuct_bt_cdcl_elimination_generator` uses the same interned tree and visit counts, but keeps dbuct's frame API: `learn()` (void; lemma from `derive_decision_lemma`), `constrain`, `push_frame`, `pop_frame`. There is no `cleanup()`.

At `learn`, the tree is interned and left **unarmed**. A raised-unit record stores the NAND, the MCTS unit boundary, and the ultimate decision. `pop_frame` undoes that frame's visit/fired journal, then either:

- still unit (`ultimate_mcts >= unit_boundary`): yields the stored ultimate and bubbles the record to the parent frame
- past the boundary: arms the NAND (`check_nand` may yield from catch-up)

A leaf counts as assigned if its visit count is live **or** `try_get(parent) == idx`. That reconstructs assignments made before the leaf existed. Size-1 lemmas never arm; they yield on every pop.

`IGetPenultimateDecision` is not used (it only seeded two-watched-literal watchers).
