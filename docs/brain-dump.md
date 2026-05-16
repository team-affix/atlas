# Brain Dump

This project has undergone countless iterations already.

Previously, the elimination backlog was thought to only be used when goals were not yet present for which their candidates have elimination known already. However, looking at the functionality of the elimination_backlog, it is evident that it served some sort of a routing function. Depending on if the goal is deactivated, active, or not yet active, it would route the elimination differently.

However, now, it is clear to me that it is an inefficiency in and of itself to first activate candidates of goals (which creates unify heads and does some unifications) to then immediately after have the elimination backlog free the eliminations for those candidates, erasing them right after we spent time initializing them. Instead, let's have the `resolver` look up eliminations in the `elimination_backlog` WHILE determining which candidates to activate. Those which are already eliminated just don't even get constructed.

Furthermore, we can promote the responsibility of elimination_backlog to be ALWAYS used, rather than just sometimes being used, when `avoidance_unit_event` is emitted. So the flow is ALWAYS `resolving_event{rl}` -> `cdcl.constrain(rl)` -> `avoidance_unit_event` -> `eb.insert(rl)` -> "resolver starts to make some goal candidates" -> `eb.contains(candidate_rl)` -> "if contains, skip candidate since we know it will be eliminated anyway"

In fact, if the elimination backlog can be treated entirely like a preventative solution, (elimination backlog never has to free eliminations or actively eliminate active candidates), then I see effectively no reason for elimination_backlog to exist in the first place. Instead, just give this responsibility to cdcl. After all, it is the thing producing unit avoidances.

And maybe that is the key, the word "Avoidance" has been telling us all along. Avoid making the candidates, it's not for eliminating them. In this case, the new flow is just `resolving_event{rl}` -> `cdcl.constrain(rl)` -> "cdcl tags the avoidance as unit" -> "resolver starts to make some goal candidates" -> `cdcl.unit(candidate_rl)` -> "if unit, skip candidate since we know it IS eliminated"

Or how about this, even better: `resolving_event{rl}` -> `cdcl.constrain(rl)` -> "resolver starts to make some goal candidates" -> `cdcl.contains({candidate_rl})` -> "if contains an avoidance with just this RL, skip candidate since we know it IS eliminated".   This way, CDCL doesn't even have to keep track of the concept of a "unit avoidance". It is only known by resolver.

This then begs the question of why CDCL should know of the concept of an "empty avoidance" or care about it. Maybe, it shouldn't. Avoidances should never become empty during the sim loop. This is because when unit avoidances exist, we avoid adding the candidate which belonged to that avoidance as an option to choose. Choosing that option would have been required to constrain cdcl onto that last candidate for the avoidance to become empty.

We do however need to pay attention to when the cdcl has directly LEARNED an empty avoidance, and to consider the solution space refuted at that point. This however can happen at the solver level.


---
### thoughts for how to update cdcl

maybe, instead of `std::unordered_map<size_t, avoidance_type>;` being the way of referencing avoidances, what if we actually allowed contraction of avoidances but kept it carefully managed?

So maybe, we could have a `std::set<avoidance_type>` for contraction and for quick lookup for `cdcl.contains(avoidance)`. Then, instead of

`std::unordered_map<const goal_lineage*, std::unordered_set<size_t>>;`

we do

`std::unordered_map<const goal_lineage*, std::unordered_set<std::set<avoidance_type>::iterator>>;`

and then a backmap

`std::unordered_map<std::set<avoidance_type>*, std::unordered_set<const goal_lineage*>>;`
(defined this way since pointer defines a hash function, and also, the pointer should always be valid regardless of iterator location since we will use extract() which preserves the original object allocation unless there is a collision, at which point we will need to handle that on re-insert)
