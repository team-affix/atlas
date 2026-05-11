#include "../../../hpp/domain/entities/goal_expr_activator.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

goal_expr_activator::goal_expr_activator(const std::vector<const expr*>& initial_exprs) :
    db(locator::locate<i_database>()),
    ef(locator::locate<i_expr_frontier>()),
    af(locator::locate<i_applicant_frontier>()),
    cp(locator::locate<i_copier>()),
    initial_exprs(initial_exprs) {}

void goal_expr_activator::start_resolution(const resolution_lineage* rl) {
    if (rl == nullptr) {
        translated_rule_body = initial_exprs;
        return;
    }

    applicant& a = af.at(rl);
    const rule& r = db.at(rl->idx);
    const expr* parent_expr = ef.at(rl->parent);
    a.u->push(parent_expr, a.copied_head);

    translated_rule_body.clear();
    for (const expr* e : r.body)
        translated_rule_body.push_back(cp.copy(e, *a.tm));
}

void goal_expr_activator::activate(const goal_lineage* gl) {
    ef.insert(gl, translated_rule_body[gl->idx]);
}
