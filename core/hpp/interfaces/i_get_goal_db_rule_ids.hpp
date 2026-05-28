#ifndef I_GET_GOAL_DB_RULE_IDS_HPP
#define I_GET_GOAL_DB_RULE_IDS_HPP

#include "../interfaces/i_rule_id_set.hpp"
#include "../value_objects/lineage.hpp"

struct i_get_goal_db_rule_ids {
    virtual ~i_get_goal_db_rule_ids() = default;
    virtual i_rule_id_set& get(const goal_lineage*) = 0;
};

#endif
