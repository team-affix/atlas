# Brain Dump

This project has undergone countless iterations already.

Previously, the elimination backlog was thought to only be used when goals were not yet present for which their candidates have elimination known already. However, looking at the functionality of the elimination_backlog, it is evident that it served some sort of a routing function. Depending on if the goal is deactivated, active, or not yet active, it would route the elimination differently.

However, now, it is clear to me that it is an inefficiency in and of itself to first activate candidates of goals (which creates unify heads and does some unifications) to then immediately after have the elimination backlog free the eliminations for those candidates, erasing them right after we spent time initializing them. Instead, let's have the `resolver` look up eliminations in the `elimination_backlog` WHILE determining which candidates to activate. Those which are already eliminated just don't even get constructed.

Furthermore, we can promote the responsibility of elimination_backlog to be ALWAYS used, rather than just sometimes being used, when `avoidance_unit_event` is emitted. So the flow is ALWAYS `resolving_event{rl}` -> `cdcl.constrain(rl)` -> `avoidance_unit_event` -> `eb.insert(rl)` -> "resolver starts to make some goal candidates" -> `eb.contains(candidate_rl)` -> "if contains, skip candidate since we know it will be eliminated anyway"

In fact, if the elimination backlog can be treated entirely like a preventative solution, (elimination backlog never has to free eliminations or actively eliminate active candidates), then I see effectively no reason for elimination_backlog to exist in the first place. Instead, just give this responsibility to cdcl. After all, it is the thing producing unit avoidances.

And maybe that is the key, the word "Avoidance" has been telling us all along. Avoid making the candidates, it's not for eliminating them.
