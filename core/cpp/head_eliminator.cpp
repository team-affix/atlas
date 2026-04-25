#include "../hpp/head_eliminator.hpp"

head_eliminator::~head_eliminator() {
    bm.set_rep_changed_callback([](uint32_t){});
}

head_eliminator::head_eliminator(
    const database& db,
    const goals& goals,
    bind_map& bm,
    expr_pool& ep,
    goal_store& gs,
    candidate_store& cs,
    lineage_pool& lp,
    std::queue<const resolution_lineage*>& unit_queue) : 
    db(db),
    bm(bm),
    ep(ep),
    gs(gs),
    cs(cs),
    lp(lp),
    fw(db, lp),
    unit_queue(unit_queue) {
    bm.set_rep_changed_callback(rep_changed_callback());
    fw.set_insert_callback(goal_inserted_callback());
    fw.set_resolve_callback(goal_resolved_callback());
    fw.initialize(goals);
}

bool head_eliminator::operator()() {
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

    // visit the goals that were touched, and if any has no candidates, return conflict.
    for (const goal_lineage* gl : touched_goals) {
        if (visit_goal_lineage(gl))
            return true;
    }

    // clear the queue since there will be invalid rep changes from
    // the temporary frames
    changed_reps = {};

    // no conflict found
    return false;
}

void head_eliminator::resolve(const resolution_lineage* r) {
    fw.resolve(r);
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

std::function<void(uint32_t)> head_eliminator::rep_changed_callback() {
    return [this](uint32_t rep) {
        changed_reps.push(rep);
    };
}

std::function<void(const goal_lineage*)> head_eliminator::goal_inserted_callback() {
    return [this](const goal_lineage* gl) {
        visit_goal_lineage(gl);
        std::unordered_set<uint32_t> reps;
        extract_rep_vars(gs.at(gl), reps);
        watch(reps, {gl});
    };
}

std::function<void(const resolution_lineage*)> head_eliminator::goal_resolved_callback() {
    return [this](const resolution_lineage* r) {
        unwatch(r->parent);
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

bool head_eliminator::visit_goal_lineage(const goal_lineage* gl) {
    // get the candidates for this goal
    std::unordered_set<size_t>& candidates = cs.at(gl);

    // get the expr for this goal
    const expr* e = gs.at(gl);

    // get if the goal WAS unit
    bool was_unit = candidates.size() == 1;

    std::erase_if(candidates, [this, e](size_t c) {
        const rule& r = db.at(c);
        return !gs.applicable(e, r);
    });

    // if newly unit, push the resolution to the unit queue
    if (!was_unit && candidates.size() == 1)
        unit_queue.push(lp.resolution(gl, *candidates.begin()));

    return candidates.empty();
}
