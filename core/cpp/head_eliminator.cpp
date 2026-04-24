#include "../hpp/head_eliminator.hpp"

head_eliminator::~head_eliminator() {
    bm.set_rep_changed_callback([](uint32_t){});
}

head_eliminator::head_eliminator(
    const database& db,
    bind_map& bm,
    expr_pool& ep,
    goal_store& gs,
    candidate_store& cs) : db(db), bm(bm), ep(ep), gs(gs), cs(cs) {
    bm.set_rep_changed_callback(slot());
}

void head_eliminator::extract_rep_vars(const expr* e, std::unordered_set<uint32_t>& reps) {
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

void head_eliminator::watch(const std::unordered_set<uint32_t>& reps, const std::unordered_set<const goal_lineage*>& goals) {
    for (uint32_t rep : reps)
        rep_to_goals[rep].insert(goals.begin(), goals.end());
    for (const goal_lineage* gl : goals)
        goal_to_reps[gl].insert(reps.begin(), reps.end());
}

std::unordered_set<const goal_lineage*> head_eliminator::unwatch(uint32_t rep) {
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

std::unordered_set<uint32_t> head_eliminator::unwatch(const goal_lineage* gl) {
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

void head_eliminator::execute() {
    std::unordered_set<const goal_lineage*> touched_goals;
    
    while (!changed_reps.empty()) {
        // pop the changed rep
        uint32_t rep = changed_reps.front();
        changed_reps.pop();

        // get the goals that are watching this representative
        auto it = rep_to_goals.find(rep);

        if (it == rep_to_goals.end())
            continue;
        
        // basic deduplication
        touched_goals.insert(it->second.begin(), it->second.end());

        // update the watches given the rep update
        update_rep_watches(rep);
    }

    for (const goal_lineage* gl : touched_goals)
        visit_goal_lineage(gl);

    // clear the queue since there will be invalid rep changes from
    // the temporary frames
    changed_reps = {};
}

std::function<void(uint32_t)> head_eliminator::slot() {
    return [this](uint32_t rep) {
        changed_reps.push(rep);
    };
}

void head_eliminator::update_rep_watches(uint32_t rep) {
    // 1. pop the goals that are watching this representative
    std::unordered_set<const goal_lineage*> goals = unwatch(rep);

    // 2. rewatch based on the new representatives
    std::unordered_set<uint32_t> new_reps;
    extract_rep_vars(ep.var(rep), new_reps);
    watch(new_reps, goals);
}

void head_eliminator::visit_goal_lineage(const goal_lineage* gl) {
    // get the candidates for this goal
    std::unordered_set<size_t>& candidates = cs.at(gl);

    // get the expr for this goal
    const expr* e = gs.at(gl);

    std::erase_if(candidates, [this, e](size_t c) {
        const rule& r = db.at(c);
        return !gs.applicable(e, r);
    });
}
