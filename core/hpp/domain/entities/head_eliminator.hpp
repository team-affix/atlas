#ifndef REP_WATCH_HPP
#define REP_WATCH_HPP

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include "lineage.hpp"
#include "expr.hpp"
#include "bind_map.hpp"
#include "goal_store.hpp"
#include "candidate_store.hpp"
#include "topic.hpp"

struct head_eliminator {
    head_eliminator();
    bool operator()();
#ifndef DEBUG
private:
#endif
    void extract_rep_vars(const expr*, std::unordered_set<uint32_t>&);
    void watch(const std::unordered_set<uint32_t>&, const std::unordered_set<const goal_lineage*>&);
    std::unordered_set<const goal_lineage*> unwatch(uint32_t);
    std::unordered_set<uint32_t> unwatch(const goal_lineage*);
    bool flush_rep_changed();
    bool flush_goal_inserted();
    void flush_goal_resolved();
    void update_rep_watches(uint32_t);
    bool visit_goal_lineage(const goal_lineage*);

    const database& db;
    bind_map& bm;
    expr_pool& ep;
    goal_store& gs;
    candidate_store& cs;
    lineage_pool& lp;

    topic<uint32_t>::subscription rep_changed_subscription;
    topic<const goal_lineage*>::subscription goal_inserted_subscription;
    topic<const resolution_lineage*>::subscription goal_resolved_subscription;
    topic<const resolution_lineage*>& unit_topic;
    std::unordered_map<uint32_t, std::unordered_set<const goal_lineage*>> rep_to_goals;
    std::unordered_map<const goal_lineage*, std::unordered_set<uint32_t>> goal_to_reps;
};

#endif
