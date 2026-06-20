#ifndef DB_HPP
#define DB_HPP

#include <algorithm>
#include <vector>
#include "infrastructure/rule_id_set.hpp"
#include "interfaces/i_get_rule.hpp"
#include "interfaces/i_push_db_rule.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/lineage.hpp"

struct db : i_get_rule, i_push_db_rule {
    db();
    rule_id push(rule r) override;
    const rule* get(rule_id) const override;
    rule_id_set& get(const goal_lineage*);
private:
    std::vector<rule> rules_;
    rule_id_set total_rule_set_;
};

namespace {

inline uint32_t max_var_count_in_expr(const expr* e) {
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

inline uint32_t compute_var_count(const rule& r) {
    uint32_t m = max_var_count_in_expr(r.head);
    for (const expr* e : r.body)
        m = std::max(m, max_var_count_in_expr(e));
    return m;
}

} // namespace

inline db::db() = default;

inline rule_id db::push(rule r) {
    if (r.var_count == 0)
        r.var_count = compute_var_count(r);
    rule_id id = rules_.size();
    rules_.push_back(std::move(r));
    total_rule_set_.insert(id);
    return id;
}

inline const rule* db::get(rule_id id) const {
    return &rules_.at(id);
}

inline rule_id_set& db::get(const goal_lineage* /*gl*/) {
    return total_rule_set_;
}

#endif
