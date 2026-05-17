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

---

May 17
Oh my gosh, I just had the craziest set of revelations in the shower. With the most massive implications for this project:

When contemplating how to properly handle the separation of concerns between constructing goals, constructing candidates, and instantiating their info, as well as adding unification heads, etc, I realized something:

maybe a candidate IS the expand_ctx...

1. Due to the fact that we just use the goal expression exactly once (seeding the unify_head), we have no need to actually preserve it in the frontier, and since that was the only actual member of goal, this means we do not need to store goals in the frontier persistently.
2. Candidates would still be present in the frontier, since their translation map is still needed until a candidate is chosen to resolve.

This got me thinking: maybe, we only need to store candidates in the frontier and goals (which seem to still be needed to carry information) are just an intermediary thing. Which led me to think:

1. This SOUNDS like resolution OF CANDIDATES. Hear me out, in order to do creation of subgoals, we ONLY need the candidate which is resolving, since all we need to do is copy the exprs of the rule body according to the translation map (stored in the parent candidate), and any derived values that the goals may have (e.g. weight, etc) can be propagated to the candidates for that goal, meaning that information can be made available in the parent candidate as well. Then, we have a bunch of child goals of the candidate, but those only contain an expr (and some extended info like goal weight), so we may just use those goals to construct candidates, and then let them fall off the stack. Then, the candidates need to be stored in the frontier. (the whole process goes from `candidate` -> `candidates`). WE ARE RESOLVING CANDIDATES NOT GOALS
2. Maybe, if we reorganize things to be *candidate-centric*, the "initial null resolution lineage" which signals the start of the simulation, would actually become an "initial null goal lineage", and (this is very important) there is no more conditional logic needed for expanding initial goals vs. later goals. This is because we can store a root candidate as the only candidate in the frontier, before sim start, and then when the sim starts, it will have to choose that candidate. The `rl` of that candidate will be referencing the rule whose body IS THE INITIAL GOALS, and the translation_map in the candidate can be empty (since there was no head for this rule). Then, that rule body (as all rule bodies do) gets copied via the empty translation map to produce the initial goals. NOTICE: we never needed to check during expansion: "is this an initial goal or an intermediate goal?"

Also, I thought:

1. "What about goals? do we ever need goals or goal lineages to be stored anywhere?" and my mind was brought to the idea of the `deactivated_goal_memory` used for the elimination router. It checks to see "is this goal active? if so, eliminate in the frontier. If deactivated, ignore elim, and if not yet active, store in backlog." However, I thought about, what if we change it to `deactivated_candidate_memory`, and then look it up there? Check it out, this is awesome. If we say that `rl` is eliminated, we look it up first in `deactivated_candidate_memory`. If it is there, then we dont do anything. Else we see if it is in the frontier, and if so, we just eliminate THAT candidate. Notice that this kills two birds with one stone: the issue of deduplicating eliminations (dont eliminate a candidate twice) AND the issue of checking "is the goal active? / on the frontier".

2. When choosing a candidate to resolve, we will need to know the parent goal lineage since we need to deactivate all candidates for that same goal. Maybe for this reason, we just have as our frontier: `map<const goal_lineage*, map<size_t, candidate>>`. Also, this will be relevant for tracking when conflicts happen, since we will need to know when a single entry in this map has no candidates.
