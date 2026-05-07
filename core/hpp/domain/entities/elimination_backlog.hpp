#ifndef ELIMINATION_BACKLOG_HPP
#define ELIMINATION_BACKLOG_HPP

#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_elimination_backlog.hpp"
#include "../../utility/tracked.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/backlogged_elimination_freed_event.hpp"

struct elimination_backlog : i_elimination_backlog {
    elimination_backlog();
    void insert(const resolution_lineage*) override;
    void goal_activated(const goal_lineage*) override;
private:
    i_event_producer<backlogged_elimination_freed_event>& backlogged_elimination_freed_producer;

    using backlog_type = std::unordered_map<const goal_lineage*, std::unordered_set<size_t>>;
    
    tracked<backlog_type> backlog;
};

#endif
