#include <stdexcept>
#include "infrastructure/db.hpp"

db::db() = default;

rule_id db::push(rule r) {
    if (std::holds_alternative<expr::var>(r.head->content))
        throw std::invalid_argument("rule head must be a functor");
    if (r.var_count == 0)
        r.var_count = compute_var_count(r);
    const uint32_t functor_id = std::get<expr::functor>(r.head->content).id;
    rule_id id = rules_.size();
    rules_.push_back(std::move(r));
    total_rule_set_.insert(id);
    functor_indexed_rule_sets_[functor_id].insert(id);
    return id;
}

const rule* db::get_rule(rule_id id) const {
    return &rules_.at(id);
}

rule_id_set& db::lookup_all_rules() {
    return total_rule_set_;
}

const rule_id_set& db::lookup_rule_by_outermost_functor(uint32_t functor_id) const {
    auto it = functor_indexed_rule_sets_.find(functor_id);
    if (it == functor_indexed_rule_sets_.end())
        return empty_rule_set_;
    return it->second;
}

uint32_t db::max_var_count_in_expr(const expr* e) {
    if (const expr::var* v = std::get_if<expr::var>(&e->content))
        return v->index + 1;
    if (const expr::functor* f = std::get_if<expr::functor>(&e->content)) {
        uint32_t m = 0;
        for (const expr* arg : f->args)
            m = std::max(m, max_var_count_in_expr(arg));
        return m;
    }
    return 0;
}

uint32_t db::compute_var_count(const rule& r) {
    uint32_t m = max_var_count_in_expr(r.head);
    for (const expr* e : r.body)
        m = std::max(m, max_var_count_in_expr(e));
    return m;
}
