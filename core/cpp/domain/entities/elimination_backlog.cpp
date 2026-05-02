#include "../../../hpp/domain/entities/elimination_backlog.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

elimination_backlog::elimination_backlog()
    :
    backlogged_elimination_freed_producer(resolver::resolve<i_event_producer<backlogged_elimination_freed_event>>()) {
}

void elimination_backlog::insert(const resolution_lineage* rl) {
    // get the parent goal
    const goal_lineage* gl = rl->parent;

    // add the candidate to the backlog
    backlog[gl].candidates.insert(rl->idx);
}

void elimination_backlog::goal_activated(const goal_lineage* gl) {
    auto node = backlog.extract(gl);

    // if the goal is not in the elimination backlog, do nothing
    if (node.empty())
        return;

    // for each index in the backlog, eliminate the candidate
    for (size_t idx : node.mapped().candidates)
        backlogged_elimination_freed_producer.produce(backlogged_elimination_freed_event{gl, idx});
}

void elimination_backlog::clear() {
    backlog.clear();
}
