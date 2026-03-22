#include "../hpp/cdcl.hpp"

cdcl::cdcl() :
    avoidances(),
    watched_goals(),
    is_refuted(false),
    eliminated_resolutions(),
    next_avoidance_id(0) {

}

void cdcl::learn(const decision_store& ds) {
    // 1. reduce the decision store to a set of leaf resolutions
    avoidance av = reduce(ds);

    // 2. get a new id for the avoidance
    size_t id = next_avoidance_id++;

    // 3. if the avoidance is empty, we are refuted.
    //    we should never achieve refutation in upsert() since we should never make
    //    moves that are eliminated which would lead to refutation.
    if (av.empty())
        is_refuted = true;
    
    // 4. add the avoidance to the store
    upsert(id, av);

    // 5. get all the goals that are watching this avoidance
    //    and link the avoidance to the goals
    for (const resolution_lineage* rl : av)
        watched_goals[rl->parent].insert(id);

}

void cdcl::constrain(const resolution_lineage* rl) {
    // 1. get the set of avoidances that concern this resolution
    std::set<size_t> ids = watched_goals[rl->parent];

    // 2. for each avoidance, if the avoidance contains the resolution,
    //    reduce the avoidance. Else, remove the avoidance from the store.
    for (size_t id : ids) {

        // 3. get the avoidance
        avoidance av = avoidances.at(id);

        // 4. reduce the avoidance
        size_t erased = av.erase(rl);

        // 5a. if the resolution was found, then it is consistent
        if (erased > 0)
            upsert(id, av);

        // 5b. if the resolution was not found, then it is conflicting
        else
            erase(id);
    }
}

void cdcl::upsert(size_t id, const avoidance& av) {
    // 1. update the avoidance in the store
    avoidances[id] = av;

    // 2. if the avoidance is singleton, we have eliminated something.
    if (av.size() == 1)
        eliminated_resolutions.insert(*av.begin());
}

void cdcl::erase(size_t id) {
    // 1. get the avoidance
    const avoidance& av = avoidances.at(id);

    // 2. for each goal that is watching this avoidance,
    //    unlink the avoidance from the goal
    for (const resolution_lineage* rl : av)
        watched_goals[rl->parent].erase(id);
    
    // 3. remove the avoidance from the store
    avoidances.erase(id);
}

avoidance cdcl::reduce(const decision_store& ds) {
    // 1. create sets of visited lineages
    std::set<const resolution_lineage*> visited;

    // 2. create result avoidance
    avoidance av = ds;

    // 3. iterate over ds, and for each entry, remove all ancestors from av
    for (const resolution_lineage* rl : ds)
        remove_ancestors(rl, av, visited);

    // 4. return the avoidance
    return av;
    
}

void cdcl::remove_ancestors(const resolution_lineage* rl, avoidance& av, std::set<const resolution_lineage*>& visited) {
    while (rl) {
        // 1. get grandparent
        //    (double-dereference safe because resolutions should
        //     never have null parent goals)
        const resolution_lineage* grandparent = rl->parent->parent;
        
        // 2. check grandparent visited
        if (visited.contains(grandparent))
            break;

        // 3. visit grandparent
        visited.insert(grandparent);

        // 4. remove grandparent from av
        av.erase(grandparent);

        // 5. rl = grandparent
        rl = grandparent;

    }
}

bool cdcl::refuted() const {
    return is_refuted;
}

bool cdcl::eliminated(const resolution_lineage* rl) const {
    return eliminated_resolutions.contains(rl);
}
