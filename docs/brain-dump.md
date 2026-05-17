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

also, we can get rid of the cdcl_sequencer since we arent using ids anymore

---

OMG this means that cdcl NEVER eliminates things already in the frontier. Therefore, we don't have to worry about synchronization problems between the eliminators since there is really only one active eliminator (multihead_unifier).

This is the craziest set of derivations ever. I believe now, that cdcl and multihead_eliminator are EXTREMELY symmetrical, and becoming moreso as time progresses. Both have a single bimap as well as 1 extra container. They both only emit 1 event type (their respective yield events). The only major difference I can see right now are quite symmetrical as well -- `multihead_eliminator` starts off empty at start of sim. CDCL starts off with TONS of entries.

I realized the following: I believe multihead_eliminator will be the only thing to ever eliminate active candidates. This is because, if you think about it, cdcl acts before candidates are active, and any future negative monitors could also not even touch the candidates directly, but rather spawn in a new solver at the current location in the tree, and see if we can solve for the negative case. Afterwards, restoring state to this location. But even this doesn't talk about eliminating specific candidates. It is for the current location.

Due to this fact though, it means that the multihead_unifier has SOLE discretion over the 



wait could what I am writing currently be wrong? I think so. Imagine the case where there is an avoidance `{A, B}`, a goal already with multiple candiates `{A,B}`. Then, there is a new resolution which takes place `B`. `A` is now eliminated, due to CDCL. But we don't just want to avoid activating that candidate under the new child goals of the new resolution, we actually want to eliminate the existing mentions as well, since those are now untenable.

Luckily, this still doesn't mean that cdcl has to remove from the frontier. In fact, it might make things even cleaner. Suppose the following symmetry: After doing a resolution, some new candidates are known to be eliminated. This is true BOTH of multihead_unifier AND cdcl. Therefore, we could do something where before activating any goals or anything, we invoke `cdcl.constrain(rl)` and `mhu.accept(rl)`. Then, each of those STORE the resolutions which are now eliminated. They both make this accessible thru `contains(rl)`. Then, before activating any new goals, etc., we do a pass over the current goals and candidates, eliminating any that are now forbidden by either eliminator.

This is the symmetry. A resolution happens, both eliminators now believe some candidates should be avoided. Resolver removes existing candidates like this, and also has a persistent lookup mechanism, avoiding creating future candidates if they are already eliminated.

Maybe, we can abstract this away from resolver, thru an `i_eliminator` class, since they both define similar methods `contains(rl)` and `constrain(rl)/accept(rl)`. Also, it seems beyond the scope of resolver's care HOW the eliminations come about. Plus, if somehow we come up with more eliminators in the future, we can just stack them using an `eliminator_stack`.

Oh in fact, a further improvement, what if resolver IS the thing that stores ALL prior eliminations for avoiding going forward? This way, it can avoid creating candidates, and also, the individual eliminators in the `eliminator_stack` don't need to each store persistent copies of the candidates already eliminated.

Actually, I don't think we can have that since initial eliminations that come about during `cdcl.learn()` would have to exist in the resolver, meaning the resolver would need to be backtrackable using trail, which is ugly. Instead, let's have an actual `avoidance_store` object which says the things to avoid doing going forward, and it is backtrackable. Maybe, let it be a repository.

Another thing: if we really wish to make `cdcl` and `multihead_unifier` not emit events (infrastructure), then they must return the eliminations thru the return value on `constrain(rl)`. And since `constrain` will be a coroutine, it means we will need to create a generator coroutine to handle it. This means we will effectively stream out eliminations one at a time, yielding. This is exactly the behavior we want, since we want to provide time for early conflicts to be detected before we eagerly eliminate all candidates that can be.

---

May 16 exactly midnight

I am considering ditching domain-driven design for Atlas. It has caused me many more problems than it has solved with the explosion of control flow everywhere (even though a solver has a fairly describably control flow), the infrastructure involved in scheduling / managing event busses / cancellation / priorities / etc. is just too much to handle for a solver like this.

My feeling when discovering that 


---

Okay so I think we should separate concerns between inserting into the frontier / other places and allocating/initializing the structures (goal/candidate)


