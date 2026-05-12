#include "../../../hpp/domain/entities/unify_synchronizer.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

unify_synchronizer::unify_synchronizer() :
    u(locator::locate<i_unifier>()),
    af(locator::locate<i_applicant_frontier>()),
    ges(locator::locate<i_expr_frontier>()),
    ep(locator::locate<i_expr_pool>()) {}

void unify_synchronizer::primary_rep_changed(uint32_t var) {
    auto it = var_to_rls.find(var);
    if (it == var_to_rls.end()) return;
    const expr* var_expr = ep.var(var);
    for (const resolution_lineage* rl : it->second)
        af.at(rl).u->push(var_expr, u.whnf(var_expr));
    update_rep_watches(var);
}

void unify_synchronizer::register_candidate(const resolution_lineage* rl) {
    std::unordered_set<uint32_t> vars;
    extract_rep_vars(ges.at(rl->parent), vars);
    watch(vars, {rl});
    for (uint32_t v : vars) {
        const expr* var_expr = ep.var(v);
        af.at(rl).u->push(var_expr, u.whnf(var_expr));
    }
}

void unify_synchronizer::unregister_candidate(const resolution_lineage* rl) {
    unwatch(rl);
}

void unify_synchronizer::accept(const resolution_lineage* rl) {
    auto it = rl_to_vars.find(rl);
    if (it == rl_to_vars.end()) return;
    for (uint32_t var : it->second) {
        const expr* var_expr = ep.var(var);
        u.push(var_expr, af.at(rl).u->whnf(var_expr));
    }
}

void unify_synchronizer::extract_rep_vars(const expr* e, std::unordered_set<uint32_t>& vars) const {
    const expr* rep = u.whnf(e);
    if (const expr::var* v = std::get_if<expr::var>(&rep->content)) {
        vars.insert(v->index);
    } else {
        const expr::functor& f = std::get<expr::functor>(rep->content);
        for (const expr* arg : f.args)
            extract_rep_vars(arg, vars);
    }
}

void unify_synchronizer::watch(const std::unordered_set<uint32_t>& vars, const std::unordered_set<const resolution_lineage*>& rls) {
    for (uint32_t var : vars)
        var_to_rls[var].insert(rls.begin(), rls.end());
    for (const resolution_lineage* rl : rls)
        rl_to_vars[rl].insert(vars.begin(), vars.end());
}

std::unordered_set<const resolution_lineage*> unify_synchronizer::unwatch(uint32_t var) {
    auto it = var_to_rls.find(var);
    for (const resolution_lineage* rl : it->second) {
        auto& vars = rl_to_vars.at(rl);
        vars.erase(var);
        if (vars.empty()) rl_to_vars.erase(rl);
    }
    auto node = var_to_rls.extract(it);
    return std::move(node.mapped());
}

std::unordered_set<uint32_t> unify_synchronizer::unwatch(const resolution_lineage* rl) {
    auto it = rl_to_vars.find(rl);
    if (it == rl_to_vars.end()) return {};
    for (uint32_t var : it->second) {
        auto& rls = var_to_rls.at(var);
        rls.erase(rl);
        if (rls.empty()) var_to_rls.erase(var);
    }
    auto node = rl_to_vars.extract(it);
    return std::move(node.mapped());
}

void unify_synchronizer::update_rep_watches(uint32_t var) {
    std::unordered_set<const resolution_lineage*> rls = unwatch(var);
    std::unordered_set<uint32_t> new_vars;
    extract_rep_vars(ep.var(var), new_vars);
    watch(new_vars, rls);
}
