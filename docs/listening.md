# Listening

elimination_backlog specifically needs to listen for when goal_activat**ed** happens, so that we know that all candidates are fully initialized and in-place before we free any backlogged eliminations for that goal. This is because, those elimination will be treated as active eliminations, which mean they need to exist first.

~~cdcl must listen for resolv**ed** event, since it emits avoidance_unit_events which~~ actually, since they just go to the backlog right away, this is acceptable. Maybe actually, we can do away with the router then and just always route to the backlog immediately, and then let the backlog decide when to apply those eliminations.
