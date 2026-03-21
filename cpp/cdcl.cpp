#include "../hpp/cdcl.hpp"

size_t cdcl::insert(const avoidance& av) {
    // 1. get a new id for the avoidance
    size_t id = avoidances.size();

    // 2. if the avoidance is empty, we are refuted.
    //    we should never achieve refutation in upsert() since we should never make
    //    moves that are eliminated which would lead to refutation.
    if (av.empty())
        is_refuted = true;
    
    // 3. add the avoidance to the store
    upsert(id, av);

    // 4. get all the goals that are watching this avoidance
    //    and link the avoidance to the goals
    for (const resolution_lineage* rl : av)
        watched_goals[rl->parent].insert(id);

    // 5. return the id
    return id;

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

bool cdcl::refuted() const {
    return is_refuted;
}

bool cdcl::eliminated(const resolution_lineage* rl) const {
    return eliminated_resolutions.contains(rl);
}
