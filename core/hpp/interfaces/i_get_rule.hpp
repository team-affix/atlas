#ifndef I_GET_RULE_HPP
#define I_GET_RULE_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/rule.hpp"

struct i_get_rule {
    virtual ~i_get_rule() = default;
    virtual const rule* get(rule_id) const = 0;
};

#endif
