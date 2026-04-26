#include "../hpp/cdcl.hpp"

cdcl::cdcl(topic<const resolution_lineage*>& new_eliminated_resolution_topic) :
    new_eliminated_resolution_topic(new_eliminated_resolution_topic),
    avoidances(),
    watched_goals(),
    is_refuted(false),
    eliminated_resolutions(),
    next_avoidance_id(0) {

}

void cdcl::learn(const lemma& l) {
    // 1. copy the already-trimmed resolutions into a local avoidance
    avoidance av = l.get_resolutions();

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
    // 1. get the parent goal
    const goal_lineage* gl = rl->parent;
    
    // 2. get the set of avoidances that concern this resolution
    std::set<size_t> ids = watched_goals[gl];

    // 3. for each avoidance, if the avoidance contains the resolution,
    //    reduce the avoidance. Else, remove the avoidance from the store.
    for (size_t id : ids) {

        // 4. get the avoidance
        avoidance av = avoidances.at(id);

        // 5. reduce the avoidance
        size_t erased = av.erase(rl);

        // 6a. if the resolution was found, then it is consistent
        if (erased > 0)
            upsert(id, av);

        // 6b. if the resolution was not found, then it is conflicting
        else
            erase(id);
    }

    // 7. Remove the parent goal from the watched goals
    watched_goals.erase(gl);
}

void cdcl::upsert(size_t id, const avoidance& av) {
    // 1. update the avoidance in the store
    avoidances[id] = av;

    // 2. if the avoidance is singleton, we have eliminated something.
    if (av.size() == 1) {
        auto [_, inserted] = eliminated_resolutions.insert(*av.begin());

        // 3. if the elimination was new, call the callback
        if (inserted)
            new_eliminated_resolution_topic.produce(*av.begin());
    }
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

const std::set<const resolution_lineage*>& cdcl::get_eliminated_resolutions() const {
    return eliminated_resolutions;
}
