#ifndef I_GET_RESOLUTION_RULE_HPP
#define I_GET_RESOLUTION_RULE_HPP

#include "../value_objects/lineage.hpp"
#include "../value_objects/rule.hpp"

struct i_get_resolution_rule {
    virtual ~i_get_resolution_rule() = default;
    virtual const rule* get(const resolution_lineage*) const = 0;
};

#endif
