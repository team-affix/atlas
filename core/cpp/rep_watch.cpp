#include "../hpp/rep_watch.hpp"

rep_watch::rep_watch(bind_map& bm, expr_pool& ep) : bm(bm), ep(ep) {

}

void rep_watch::extract_rep_vars(const expr* e, std::unordered_set<uint32_t>& reps) {
    const expr* e_rep = bm.whnf(e);

    if (const expr::var* v = std::get_if<expr::var>(&e_rep->content)) {
        reps.insert(v->index);
    }
    else {
        const expr::functor& f = std::get<expr::functor>(e_rep->content);
        for (const expr* arg : f.args)
            extract_rep_vars(arg, reps);
    }
    
}

void rep_watch::watch(const std::unordered_set<uint32_t>& reps, const std::unordered_set<const goal_lineage*>& goals) {
    for (uint32_t rep : reps)
        rep_to_goals[rep].insert(goals.begin(), goals.end());
    for (const goal_lineage* gl : goals)
        goal_to_reps[gl].insert(reps.begin(), reps.end());
}

std::unordered_set<const goal_lineage*> rep_watch::unwatch(uint32_t rep) {
    auto it = rep_to_goals.find(rep);
    for (const goal_lineage* gl : it->second) {
        auto reps = goal_to_reps.at(gl);
        reps.erase(rep);
        if (reps.empty())
            goal_to_reps.erase(gl);
    }
    auto node = rep_to_goals.extract(it);
    return std::move(node.mapped());
}

std::unordered_set<uint32_t> rep_watch::unwatch(const goal_lineage* gl) {
    auto it = goal_to_reps.find(gl);
    for (uint32_t rep : it->second) {
        auto goals = rep_to_goals.at(rep);
        goals.erase(gl);
        if (goals.empty())
            rep_to_goals.erase(rep);
    }
    auto node = goal_to_reps.extract(it);
    return std::move(node.mapped());
}

std::unordered_set<const goal_lineage*> rep_watch::pipe() {
    std::unordered_set<const goal_lineage*> result;
    std::unordered_set<uint32_t> changed_reps = bm.flush_changed_reps();
    for (uint32_t rep : changed_reps) {
        if (rep_to_goals.contains(rep))
            update_rep_watches(rep);
    }
    return result;
}

void rep_watch::update_rep_watches(uint32_t rep) {
    // 1. pop the goals that are watching this representative
    std::unordered_set<const goal_lineage*> goals = unwatch(rep);

    // 2. rewatch based on the new representatives
    std::unordered_set<uint32_t> new_reps;
    extract_rep_vars(ep.var(rep), new_reps);
    watch(new_reps, goals);
}
