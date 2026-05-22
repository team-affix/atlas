#include <memory>
#include "../../hpp/infrastructure/cdcl_elimination_generator.hpp"
#include "../../hpp/utility/backtrackable_set_insert.hpp"
#include "../../hpp/utility/backtrackable_set_erase.hpp"
#include "../../hpp/utility/backtrackable_map_insert.hpp"
#include "../../hpp/utility/backtrackable_map_erase.hpp"
#include "../../hpp/utility/backtrackable_map_at_insert.hpp"
#include "../../hpp/utility/backtrackable_map_at_erase.hpp"

cdcl_elimination_generator::cdcl_elimination_generator(i_trail& trail) :
    avoidances(trail, {}),
    watched_goals(trail, {}) {
}

const resolution_lineage* cdcl_elimination_generator::learn(const lemma& l) {
    // 1. get the resolutions
    const auto& resolutions = l.get_resolutions();
    
    // 2. copy the already-trimmed resolutions into a local avoidance
    avoidance_type av{resolutions.begin(), resolutions.end()};

    // NOTE: No need to check if the avoidance is already in the store,
    //       because the avoidance should always be a new one.
    
    // 3. insert the avoidance into the store
    return insert(av);
}

state_machine<const resolution_lineage*> cdcl_elimination_generator::constrain(const resolution_lineage* rl) {
    // 1. get the parent goal
    const goal_lineage* gl = rl->parent;
    
    // 2. get the set of avoidances that concern this resolution
    const std::unordered_set<const avoidance_type*>& av_ptrs = watched_goals.get().at(gl);

    // 3. for each avoidance, if the avoidance contains the resolution,
    //    reduce the avoidance. Else, remove the avoidance from the store.
    for (const avoidance_type* av_ptr : av_ptrs) {
        // 4. if the avoidance does not contain the resolution,
        //    then it is mutually exclusive with the resolution
        if (!av_ptr->contains(rl)) {
            erase(av_ptr);
            continue;
        }

        // if the avoidance contains the resolution,
        // then it is consistent with the resolution

        // 5. make a copy of the avoidance
        avoidance_type av = *av_ptr;

        // 6. erase and unlink the old avoidance from the store
        erase(av_ptr);

        // 7. reduce the avoidance
        av.erase(rl);

        // 8. insert the reduced avoidance into the store
        if (const resolution_lineage* rl = insert(av))
            co_yield rl;
    }

    // 7. Remove the parent goal from the watched goals
    auto erase_mut = std::make_unique<
        backtrackable_map_erase<
        watched_goals_type>>(
            gl);
    watched_goals.mutate(std::move(erase_mut));
}

const resolution_lineage* cdcl_elimination_generator::insert(const avoidance_type& av) {
    // 1. if avoidance unit, elimination
    if (av.size() == 1)
        return *av.begin();

    // 2. if avoidance already in store, dont overwrite,
    //    just return since same goals would already be linked
    //    to that avoidance as well (deduplication)
    if (avoidances.get().contains(av))
        return nullptr;

    // 3. add the new avoidance to the store
    auto insert_mut = std::make_unique<
        backtrackable_set_insert<
        std::set<avoidance_type>>>(
            av);
    avoidances.mutate(std::move(insert_mut));

    // 4. get the pointer to the new avoidance
    const avoidance_type* new_av_ptr = &*avoidances.get().find(av);

    // 5. link the new avoidance to the goals
    for (const resolution_lineage* rl : av)
        link(rl->parent, new_av_ptr);

    // 6. return null since no elimination
    return nullptr;
}

void cdcl_elimination_generator::link(const goal_lineage* gl, const avoidance_type* av_ptr) {
    // insert key if it doesn't exist
    if (!watched_goals.get().contains(gl)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_insert<
            watched_goals_type>>(
                gl, std::unordered_set<const avoidance_type*>{});
        watched_goals.mutate(std::move(insert_mut));
    }

    // insert value if it doesn't exist
    if (!watched_goals.get().at(gl).contains(av_ptr)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_at_insert<
            watched_goals_type>>(
                gl, av_ptr);
        watched_goals.mutate(std::move(insert_mut));
    }
}

void cdcl_elimination_generator::erase(const avoidance_type* av_ptr) {
    // 1. for each goal that is watching this avoidance,
    //    unlink the avoidance from the goal
    for (const resolution_lineage* rl : *av_ptr) {
        auto erase_mut = std::make_unique<
            backtrackable_map_at_erase<
            watched_goals_type>>(
                rl->parent, av_ptr);
        watched_goals.mutate(std::move(erase_mut));
    }
    
    // 2. remove the avoidance from the store
    auto erase_mut = std::make_unique<
        backtrackable_set_erase<
        std::set<avoidance_type>>>(
            *av_ptr);
    avoidances.mutate(std::move(erase_mut));
}
