#ifndef ELIMINATION_BACKLOG_HPP
#define ELIMINATION_BACKLOG_HPP

#include <unordered_map>
#include "../interfaces/i_elimination_backlog.hpp"
#include "../interfaces/i_candidate_store.hpp"
#include "../interfaces/i_active_goal_store.hpp"
#include "../interfaces/i_inactive_goal_store.hpp"

struct elimination_backlog : i_elimination_backlog {
    elimination_backlog();
    void insert(const resolution_lineage*) override;
    void goal_activated(const goal_lineage*) override;
#ifndef DEBUG
private:
#endif
    i_candidate_store& cs;
    i_active_goal_store& active_goal_store;
    i_inactive_goal_store& inactive_goal_store;
    
    std::unordered_map<const goal_lineage*, candidate_set> backlog;
};

#endif
