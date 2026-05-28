#ifndef DB_HPP
#define DB_HPP

#include <vector>
#include "../interfaces/i_get_goal_db_rule_ids.hpp"
#include "../interfaces/i_get_rule.hpp"
#include "../value_objects/rule.hpp"
#include "rule_id_set.hpp"

struct db
    : i_get_goal_db_rule_ids
    , i_get_rule {
    db();
    rule_id push(rule r);
    const rule* get(rule_id) const override;
    i_rule_id_set& get(const goal_lineage*) override;
private:
    std::vector<rule> rules_;
    rule_id_set total_rule_set_;
};

#endif
