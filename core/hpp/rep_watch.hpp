#ifndef REP_WATCH_HPP
#define REP_WATCH_HPP

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include "lineage.hpp"
#include "expr.hpp"
#include "bind_map.hpp"

struct rep_watch {
    rep_watch(bind_map&, expr_pool&);
    void extract_rep_vars(const expr*, std::unordered_set<uint32_t>&);
    void watch(const std::unordered_set<uint32_t>&, const std::unordered_set<const goal_lineage*>&);
    std::unordered_set<const goal_lineage*> unwatch(uint32_t);
    std::unordered_set<uint32_t> unwatch(const goal_lineage*);
    std::unordered_set<const goal_lineage*> pipe();
#ifndef DEBUG
private:
#endif
    void update_rep_watches(uint32_t);

    bind_map& bm;
    expr_pool& ep;

    std::unordered_map<uint32_t, std::unordered_set<const goal_lineage*>> rep_to_goals;
    std::unordered_map<const goal_lineage*, std::unordered_set<uint32_t>> goal_to_reps;
};

#endif
