#include "../../../hpp/domain/entities/elimination_backlog.hpp"
#include "../../../hpp/bootstrap/locator.hpp"
#include "../../../hpp/utility/backtrackable_map_insert.hpp"
#include "../../../hpp/utility/backtrackable_map_at_insert.hpp"
#include "../../../hpp/utility/backtrackable_map_erase.hpp"

elimination_backlog::elimination_backlog()
    :
    backlogged_elimination_freed_producer(locator::locate<i_event_producer<backlogged_elimination_freed_event>>()),
    elimination_backlog_free_yielded_producer(locator::locate<i_event_producer<elimination_backlog_free_yielded_event>>()),
    backlog(locator::locate<i_trail>(), {}) {
}

void elimination_backlog::insert(const resolution_lineage* rl) {
    // get the parent goal
    const goal_lineage* gl = rl->parent;

    // insert the goal into the backlog if it is not already present
    if (!backlog.get().contains(gl)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_insert<
            backlog_type>>(
                gl, std::unordered_set<size_t>{});
        backlog.mutate(std::move(insert_mut));
    }

    // if the candidate is not present for this goal, add it
    if (!backlog.get().at(gl).contains(rl->idx)) {
        auto insert_mut = std::make_unique<
            backtrackable_map_at_insert<
            backlog_type>>(
                gl, rl->idx);
        backlog.mutate(std::move(insert_mut));
    }
}

void elimination_backlog::init_free(const goal_lineage* gl) {
    elimination_backlog_free_state_machine = free(gl);
    elimination_backlog_free_yielded_producer.produce({});
}

void elimination_backlog::resume_free() {
    elimination_backlog_free_state_machine->resume();
    if (!elimination_backlog_free_state_machine->done())
        elimination_backlog_free_yielded_producer.produce({});
}

void elimination_backlog::discard(const goal_lineage* gl) {
    // construct the erase mutation
    auto erase_mut = std::make_unique<
        backtrackable_map_erase<
        backlog_type>>(
            gl);
    // apply the mutation
    backlog.mutate(std::move(erase_mut));
}

state_machine elimination_backlog::free(const goal_lineage* gl) {
    auto it = backlog.get().find(gl);
    
    // if the goal is not in the elimination backlog, do nothing
    if (it == backlog.get().end())
        co_return;

    // for each index in the backlog, eliminate the candidate
    for (size_t idx : it->second) {
        backlogged_elimination_freed_producer.produce(backlogged_elimination_freed_event{gl, idx});
        co_await std::suspend_always{};
    }

    // remove the goal from the backlog
    discard(gl);
}
