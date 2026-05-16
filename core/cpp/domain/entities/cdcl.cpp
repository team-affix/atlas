#include <memory>
#include "../../../hpp/domain/entities/cdcl.hpp"
#include "../../../hpp/bootstrap/locator.hpp"
#include "../../../hpp/utility/backtrackable_set_insert.hpp"
#include "../../../hpp/utility/backtrackable_map_insert.hpp"
#include "../../../hpp/utility/backtrackable_map_erase.hpp"
#include "../../../hpp/utility/backtrackable_map_at_insert.hpp"
#include "../../../hpp/utility/backtrackable_map_at_erase.hpp"

cdcl::cdcl() :
    cdcl_constrain_yielded_producer(locator::locate<i_event_producer<cdcl_constrain_yielded_event>>()),
    avoidances(locator::locate<i_trail>(), {}),
    contraction_map(locator::locate<i_trail>(), {}),
    watched_goals(locator::locate<i_trail>(), {}) {
}

void cdcl::learn(const lemma& l) {
    // 1. get the resolutions
    const auto& resolutions = l.get_resolutions();
    
    // 2. copy the already-trimmed resolutions into a local avoidance
    avoidance_type av{resolutions.begin(), resolutions.end()};

    // 3. check if the avoidance is already in the store
    if (avoidances.get().contains(av)) {
        return;
    }
    
    // 4. add the avoidance to the store
    auto insert_mut = std::make_unique<
        backtrackable_set_insert<
        avoidances_type>>(
            av);
    avoidances.mutate(std::move(insert_mut));

    // 5. get the pointer to the avoidance
    const avoidance_type* av_ptr = &*avoidances.get().find(av);

    // 4. get all the goals that are watching this avoidance
    //    and link the avoidance to the goals
    for (const resolution_lineage* rl : av)
        link(rl->parent, id);
}

void cdcl::init_constrain(const resolution_lineage* rl) {
    constrain_state_machine = constrain(rl);
    // emit yielded event
    cdcl_constrain_yielded_producer.produce({});
}

void cdcl::resume_constrain() {
    constrain_state_machine->resume();
    if (!constrain_state_machine->done())
        cdcl_constrain_yielded_producer.produce({});
}

bool cdcl::contains(const avoidance_type& av) {
    // return avoidances_map.get().contains(av);
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
    const avoidance_type& av = avoidances_map.get().at(id);

    // 2. for each goal that is watching this avoidance,
    //    unlink the avoidance from the goal
    for (const resolution_lineage* rl : av) {
        auto erase_mut = std::make_unique<
            backtrackable_map_at_erase<
            watched_goals_type>>(
                rl->parent, id);
        watched_goals.mutate(std::move(erase_mut));
    }
    
    // 3. remove the avoidance from the store
    auto erase_mut = std::make_unique<
        backtrackable_map_erase<
        avoidances_map_type>>(
            id);
    avoidances_map.mutate(std::move(erase_mut));
}

state_machine cdcl::constrain(const resolution_lineage* rl) {
    // 1. get the parent goal
    const goal_lineage* gl = rl->parent;
    
    // 2. get the set of avoidances that concern this resolution
    const std::unordered_set<size_t>& ids = watched_goals.get().at(gl);

    // 3. for each avoidance, if the avoidance contains the resolution,
    //    reduce the avoidance. Else, remove the avoidance from the store.
    for (size_t id : ids) {

        // 4. get the avoidance
        const avoidance_type& av = avoidances_map.get().at(id);

        // 5. if the avoidance contains the resolution, then it is consistent
        if (av.contains(rl)) {
            auto erase_mut = std::make_unique<
                backtrackable_map_at_erase<
                avoidances_map_type>>(
                    id, rl);
            avoidances_map.mutate(std::move(erase_mut));
            co_await std::suspend_always{};
            continue;
        }

        // 6. if the avoidance does not contain the resolution, then it is conflicting
        erase(id);
        co_await std::suspend_always{};
    }

    // 7. Remove the parent goal from the watched goals
    auto erase_mut = std::make_unique<
        backtrackable_map_erase<
        watched_goals_type>>(
            gl);
    watched_goals.mutate(std::move(erase_mut));
}
