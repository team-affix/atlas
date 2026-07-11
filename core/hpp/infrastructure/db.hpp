#ifndef DB_HPP
#define DB_HPP

#include <algorithm>
#include <vector>
#include "infrastructure/rule_id_set.hpp"
#include "value_objects/rule.hpp"
#include "value_objects/lineage.hpp"

struct db {
    db();
    rule_id push(rule r);
    const rule* get_rule(rule_id) const;
    rule_id_set& get_candidate_rules(const goal_lineage*);
private:
    static uint32_t max_var_count_in_expr(const expr* e);
    static uint32_t compute_var_count(const rule& r);
    std::vector<rule> rules_;
    rule_id_set total_rule_set_;
};

#endif
