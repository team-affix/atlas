#ifndef DB_HPP
#define DB_HPP

#include "../interfaces/i_get_goal_db_rules.hpp"
#include "rule_set.hpp"

struct db : i_get_goal_db_rules {
    db();
    explicit db(rule_set total_rules);
    i_rule_set& get(const goal_lineage*) override;
private:
    rule_set total_rules_;
};

#endif
