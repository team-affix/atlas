#ifndef I_PUSH_DB_RULE_HPP
#define I_PUSH_DB_RULE_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

struct i_push_db_rule {
    virtual ~i_push_db_rule() = default;
    virtual rule_id push(rule r) = 0;
};

#endif
