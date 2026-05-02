#include "../../../hpp/domain/entities/goal_expr_expander.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

goal_expr_expander::goal_expr_expander() :
    db(resolver::resolve<i_database>()),
    ges(resolver::resolve<i_goal_expr_store>()),
    bm(resolver::resolve<i_bind_map>()),
    cp(resolver::resolve<i_copier>()),
    representative_changed_producer(resolver::resolve<i_event_producer<representative_changed_event>>()) {
}

void goal_expr_expander::start_expansion(const resolution_lineage* rl) {
    // get the rule
    rule r = db.at(rl->idx);
    rule_body = r.body;

    // get the goal expr
    const expr* goal_expr = ges.at(rl->parent);

    // clear previous translation map
    translation_map.clear();

    // unify the goal expr with rule head
    std::queue<uint32_t> rep_changes;
    bool success = bm.unify(goal_expr, r.head, rep_changes);
    
    // should not fail
    assert(success);

    // emit events for the rep changes
    while (!rep_changes.empty()) {
        uint32_t var_index = rep_changes.front();
        rep_changes.pop();
        representative_changed_producer.produce(representative_changed_event{var_index});
    }
}

void goal_expr_expander::expand_child(const goal_lineage* gl) {
    // get the child expr from body
    const expr* rule_child = rule_body[gl->idx];

    // copy the rule child
    const expr* copied_child = cp.copy(rule_child, translation_map);

    // add the copied child to the goal expr store
    ges.insert(gl, copied_child);
}
