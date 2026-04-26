#include "../hpp/head_eliminator.hpp"

head_eliminator::head_eliminator(
    const database& db,
    const goals& goals,
    bind_map& bm,
    expr_pool& ep,
    goal_store& gs,
    candidate_store& cs,
    lineage_pool& lp,
    topic<uint32_t>& rep_changed_topic,
    topic<const goal_lineage*>& goal_inserted_topic,
    topic<const resolution_lineage*>& goal_resolved_topic,
    topic<const resolution_lineage*>& unit_topic) : 
    db(db),
    bm(bm),
    ep(ep),
    gs(gs),
    cs(cs),
    lp(lp),
    rep_changed_subscription(rep_changed_topic),
    goal_inserted_subscription(goal_inserted_topic),
    goal_resolved_subscription(goal_resolved_topic),
    unit_topic(unit_topic) {

}

bool head_eliminator::operator()() {
    // flush goals resolved first since that will be an efficiency bost
    // for the rep changed flush.
    flush_goal_resolved();
        
    if (flush_rep_changed())
        return true;
    
    if (flush_goal_inserted())
        return true;

    // clear the queue since there will be invalid rep changes from
    // the temporary frames
    rep_changed_subscription.purge();

    return false;
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
        auto& reps = goal_to_reps.at(gl);
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
        auto& goals = rep_to_goals.at(rep);
        goals.erase(gl);
        if (goals.empty())
            rep_to_goals.erase(rep);
    }
    auto node = goal_to_reps.extract(it);
    return std::move(node.mapped());
}

bool head_eliminator::flush_rep_changed() {
    std::unordered_set<const goal_lineage*> touched_goals;

    while (!rep_changed_subscription.empty()) {
        // pop the changed rep
        uint32_t rep = rep_changed_subscription.consume();

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
    
    return false;
}

bool head_eliminator::flush_goal_inserted() {
    while (!goal_inserted_subscription.empty()) {
        auto gl = goal_inserted_subscription.consume();
        if (visit_goal_lineage(gl))
            return true;
        std::unordered_set<uint32_t> reps;
        extract_rep_vars(gs.at(gl), reps);
        watch(reps, {gl});
    }
    return false;
}

void head_eliminator::flush_goal_resolved() {
    while (!goal_resolved_subscription.empty()) {
        auto rl = goal_resolved_subscription.consume();
        unwatch(rl->parent);
    }
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
        unit_topic.produce(lp.resolution(gl, *candidates.begin()));

    return candidates.empty();
}
