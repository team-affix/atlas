#include "../../../hpp/domain/entities/candidate_not_applicable_detector.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"
#include "../../../hpp/infrastructure/bind_map.hpp"

candidate_not_applicable_detector::candidate_not_applicable_detector() :
    rbms(resolver::resolve<i_resolution_bind_map_store>()),
    ges(resolver::resolve<i_goal_expr_store>()),
    db(resolver::resolve<i_database>()),
    cp(resolver::resolve<i_copier>()),
    ep(resolver::resolve<i_expr_pool>()),
    rtms(resolver::resolve<i_resolution_translation_map_store>()),
    candidate_not_applicable_producer(resolver::resolve<i_event_producer<candidate_not_applicable_event>>()) {
    rbms.insert(nullptr, std::make_unique<bind_map>(nullptr));
}

static i_bind_map& primary(i_resolution_bind_map_store& rbms) {
    return *rbms.get(nullptr);
}

void candidate_not_applicable_detector::add_candidate(const resolution_lineage* rl) {
    const goal_lineage* gl = rl->parent;

    translation_map tm;
    const expr* renamed_head = cp.copy(db.at(rl->idx).head, tm);
    rtms.insert(rl, std::move(tm));

    // Register which goal vars this secondary watches
    std::unordered_set<uint32_t> goal_vars;
    extract_vars(ges.at(gl), goal_vars);
    for (uint32_t v : goal_vars) {
        var_to_rls[v].insert(rl);
        rl_to_vars[rl].insert(v);
    }

    rbms.insert(rl, std::make_unique<bind_map>(rl));
    i_bind_map& secondary = *rbms.get(rl);

    // Seed initial unification pair
    secondary.push(ges.at(gl), renamed_head);

    // Reconcile immediately: fold in any bindings the primary already holds
    for (uint32_t v : goal_vars) {
        const expr* var_expr = ep.var(v);
        const expr* pw = primary(rbms).whnf(var_expr);
        if (pw != var_expr)
            secondary.push(var_expr, pw);
    }

    secondary.process_step();
}

void candidate_not_applicable_detector::primary_rep_changed(uint32_t var_index) {
    auto it = var_to_rls.find(var_index);
    if (it == var_to_rls.end())
        return;

    const expr* var_expr = ep.var(var_index);
    const expr* pw = primary(rbms).whnf(var_expr);

    for (const resolution_lineage* rl : it->second) {
        (*rbms.get(rl)).push(var_expr, pw);
        (*rbms.get(rl)).process_step();
    }
}

void candidate_not_applicable_detector::secondary_unify_failed(const resolution_lineage* rl) {
    candidate_not_applicable_producer.produce({rl});
}

void candidate_not_applicable_detector::remove_candidate(const resolution_lineage* rl) {
    auto vars_it = rl_to_vars.find(rl);
    if (vars_it != rl_to_vars.end()) {
        for (uint32_t v : vars_it->second)
            var_to_rls[v].erase(rl);
        rl_to_vars.erase(rl);
    }

    rbms.erase(rl);
    rtms.erase(rl);
}

void candidate_not_applicable_detector::extract_vars(
        const expr* e, std::unordered_set<uint32_t>& vars) const {
    if (const expr::var* v = std::get_if<expr::var>(&e->content)) {
        vars.insert(v->index);
    } else if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        for (const expr* arg : f->args)
            extract_vars(arg, vars);
    }
}
