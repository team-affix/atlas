#include "../../../hpp/domain/entities/goal_expr_expander.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_expr_expander::goal_expr_expander() :
    db(resolver::resolve<i_database>()),
    ges(resolver::resolve<i_goal_expr_store>()),
    bm(resolver::resolve<i_bind_map>()),
    cp(resolver::resolve<i_copier>()),
    tm_factory(resolver::resolve<i_factory<i_translation_map>>()),
    tm(tm_factory.make()) {
}

void goal_expr_expander::start_expansion(const resolution_lineage* rl) {
    rule r = db.at(rl->idx);
    rule_body = r.body;

    const expr* goal_expr = ges.at(rl->parent);
    tm->clear();

    bm.push(goal_expr, r.head);
    bm.process_step();
}

void goal_expr_expander::expand_child(const goal_lineage* gl) {
    const expr* rule_child = rule_body[gl->idx];
    const expr* copied_child = cp.copy(rule_child, *tm);
    ges.insert(gl, copied_child);
}
