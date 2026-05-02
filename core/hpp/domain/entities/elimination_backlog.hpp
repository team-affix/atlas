#ifndef ELIMINATION_BACKLOG_HPP
#define ELIMINATION_BACKLOG_HPP

#include <unordered_map>
#include "../interfaces/i_elimination_backlog.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/backlogged_elimination_freed_event.hpp"
#include "../value_objects/candidate_set.hpp"

struct elimination_backlog : i_elimination_backlog {
    elimination_backlog();
    void insert(const resolution_lineage*) override;
    void goal_activated(const goal_lineage*) override;
    void clear() override;
#ifndef DEBUG
private:
#endif
    i_event_producer<backlogged_elimination_freed_event>& backlogged_elimination_freed_producer;

    std::unordered_map<const goal_lineage*, candidate_set> backlog;
};

#endif
