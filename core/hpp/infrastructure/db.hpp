#ifndef DB_HPP
#define DB_HPP

#include <algorithm>
#include <unordered_map>
#include <vector>
#include "infrastructure/rule_id_set.hpp"
#include "value_objects/rule.hpp"

struct db {
    db();
    rule_id push(rule r);
    const rule* get_rule(rule_id) const;
    rule_id_set& lookup_all_rules();
    const rule_id_set& lookup_rule_by_outermost_functor(uint32_t functor_id) const;
private:
    static uint32_t max_var_count_in_expr(const expr* e);
    static uint32_t compute_var_count(const rule& r);
    std::vector<rule> rules_;
    rule_id_set total_rule_set_;
    std::unordered_map<uint32_t, rule_id_set> functor_indexed_rule_sets_;
    rule_id_set empty_rule_set_;
};

#endif
