#include <memory>
#include "../../../hpp/domain/entities/cdcl.hpp"
#include "../../../hpp/bootstrap/locator.hpp"
#include "../../../hpp/utility/backtrackable_map_insert.hpp"
#include "../../../hpp/utility/backtrackable_map_erase.hpp"
#include "../../../hpp/utility/backtrackable_map_at_insert.hpp"
#include "../../../hpp/utility/backtrackable_map_at_erase.hpp"

cdcl::cdcl() :
    avoidance_unit_producer(locator::resolve<i_event_producer<avoidance_unit_event>>()),
    avoidance_empty_producer(locator::resolve<i_event_producer<avoidance_empty_event>>()),
    next_avoidance_id(locator::resolve<i_cdcl_sequencer>()),
    avoidances(locator::resolve<i_trail>(), {}),
    watched_goals(locator::resolve<i_trail>(), {}) {
}

void cdcl::learn(const lemma& l) {
    // 1. copy the already-trimmed resolutions into a local avoidance
    avoidance av{l.get_resolutions()};

    // 2. get a new id for the avoidance
    size_t id = next_avoidance_id.next();

    // 3. add the avoidance to the store
    auto insert_mut = std::make_unique<
        backtrackable_map_insert<
        avoidances_type>>(
            id, av);
    avoidances.mutate(std::move(insert_mut));

    // 4. get all the goals that are watching this avoidance
    //    and link the avoidance to the goals
    for (const resolution_lineage* rl : av.decisions)
        link(rl->parent, id);

}

void cdcl::constrain(const resolution_lineage* rl) {
    // 1. get the parent goal
    const goal_lineage* gl = rl->parent;
    
    // 2. get the set of avoidances that concern this resolution
    const std::unordered_set<size_t>& ids = watched_goals.get().at(gl);

    // 3. for each avoidance, if the avoidance contains the resolution,
    //    reduce the avoidance. Else, remove the avoidance from the store.
    for (size_t id : ids) {

        // 4. get the avoidance
        const avoidance& av = avoidances.get().at(id);

        // 5. if the avoidance contains the resolution, then it is consistent
        if (av.decisions.contains(rl)) {
            auto erase_mut = std::make_unique<
                backtrackable_map_at_erase<
                watched_goals_type>>(
                    gl, id);
            watched_goals.mutate(std::move(erase_mut));
            return;
        }

        // 6. if the avoidance does not contain the resolution, then it is conflicting
        erase(id);
    }

    // 7. Remove the parent goal from the watched goals
    auto erase_mut = std::make_unique<
        backtrackable_map_erase<
        watched_goals_type>>(
            gl);
    watched_goals.mutate(std::move(erase_mut));
}

const avoidance& cdcl::get_avoidance(size_t id) {
    return avoidances.get().at(id);
}

void cdcl::updated(size_t id) {
    // 1. get the avoidance
    const avoidance& av = avoidances.get().at(id);

    // 2. if the avoidance is empty, add it to the empty avoidances
    if (av.decisions.empty()) {
        avoidance_empty_producer.produce(avoidance_empty_event{id});
    }
    // 3. if the avoidance is singleton, add it to the unit avoidances
    else if (av.decisions.size() == 1) {
        avoidance_unit_producer.produce(avoidance_unit_event{id});
    }
}

void cdcl::link(const goal_lineage* gl, size_t id) {
    // insert key if it doesn't exist
    if (!watched_goals.get().contains(gl)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_insert<
            watched_goals_type>>(
                gl, std::unordered_set<size_t>{});
        watched_goals.mutate(std::move(insert_mut));
    }

    // insert value if it doesn't exist
    if (!watched_goals.get().at(gl).contains(id)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_at_insert<
            watched_goals_type>>(
                gl, id);
        watched_goals.mutate(std::move(insert_mut));
    }
}

void cdcl::erase(size_t id) {
    // 1. get the avoidance
    const avoidance& av = avoidances.get().at(id);

    // 2. for each goal that is watching this avoidance,
    //    unlink the avoidance from the goal
    for (const resolution_lineage* rl : av.decisions) {
        auto erase_mut = std::make_unique<
            backtrackable_map_at_erase<
            watched_goals_type>>(
                rl->parent, id);
        watched_goals.mutate(std::move(erase_mut));
    }
    
    // 3. remove the avoidance from the store
    auto erase_mut = std::make_unique<
        backtrackable_map_erase<
        avoidances_type>>(
            id);
    avoidances.mutate(std::move(erase_mut));
}
