#ifndef REP_WATCH_HPP
#define REP_WATCH_HPP

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "lineage.hpp"
#include "expr.hpp"
#include "bind_map.hpp"
#include "goal_store.hpp"
#include "candidate_store.hpp"
#include "frontier_watch.hpp"

struct head_eliminator {
    ~head_eliminator();
    head_eliminator(
        const database&,
        const goals&,
        bind_map&,
        expr_pool&,
        goal_store&,
        candidate_store&,
        lineage_pool&
    );
    void execute();
#ifndef DEBUG
private:
#endif
    void extract_rep_vars(const expr*, std::unordered_set<uint32_t>&);
    void watch(const std::unordered_set<uint32_t>&, const std::unordered_set<const goal_lineage*>&);
    std::unordered_set<const goal_lineage*> unwatch(uint32_t);
    std::unordered_set<uint32_t> unwatch(const goal_lineage*);
    std::function<void(uint32_t)> rep_changed_callback();
    std::function<void(const goal_lineage*)> goal_inserted_callback();
    std::function<void(const resolution_lineage*)> goal_resolved_callback();
    void update_rep_watches(uint32_t);
    void visit_goal_lineage(const goal_lineage*);

    const database& db;
    bind_map& bm;
    expr_pool& ep;
    goal_store& gs;
    candidate_store& cs;

    frontier_watch fw;
    std::queue<uint32_t> changed_reps;
    std::unordered_map<uint32_t, std::unordered_set<const goal_lineage*>> rep_to_goals;
    std::unordered_map<const goal_lineage*, std::unordered_set<uint32_t>> goal_to_reps;
};

#endif
