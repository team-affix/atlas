#ifndef ELIMINATION_BACKLOG_HPP
#define ELIMINATION_BACKLOG_HPP

#include <optional>
#include <unordered_map>
#include <unordered_set>
#include "../interfaces/i_elimination_backlog.hpp"
#include "../../utility/tracked.hpp"
#include "../interfaces/i_event_producer.hpp"
#include "../events/backlogged_elimination_freed_event.hpp"
#include "../events/elimination_backlog_free_yielded_event.hpp"
#include "../../utility/state_machine.hpp"

struct elimination_backlog : i_elimination_backlog {
    elimination_backlog();
    void insert(const resolution_lineage*) override;
    void init_free(const goal_lineage*) override;
    void resume_free() override;
    void discard(const goal_lineage*) override;
private:
    state_machine free(const goal_lineage*);

    i_event_producer<backlogged_elimination_freed_event>& backlogged_elimination_freed_producer;
    i_event_producer<elimination_backlog_free_yielded_event>& elimination_backlog_free_yielded_producer;

    using backlog_type = std::unordered_map<const goal_lineage*, std::unordered_set<size_t>>;
    
    std::optional<state_machine> elimination_backlog_free_state_machine;
    
    tracked<backlog_type> backlog;
};

#endif
