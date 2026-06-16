#include <algorithm>
#include "infrastructure/db.hpp"

db::db() = default;

namespace {

uint32_t max_var_count_in_expr(const expr* e) {
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

uint32_t compute_var_count(const rule& r) {
    uint32_t m = max_var_count_in_expr(r.head);
    for (const expr* e : r.body)
        m = std::max(m, max_var_count_in_expr(e));
    return m;
}

} // namespace

rule_id db::push(rule r) {
    if (r.var_count == 0)
        r.var_count = compute_var_count(r);
    rule_id id = rules_.size();
    rules_.push_back(std::move(r));
    total_rule_set_.insert(id);
    return id;
}

const rule* db::get(rule_id id) const {
    return &rules_.at(id);
}

i_rule_id_set& db::get(const goal_lineage* /*gl*/) {
    return total_rule_set_;
}
