#ifndef I_RA_RULE_ID_SET_HPP
#define I_RA_RULE_ID_SET_HPP

#include "interfaces/i_rule_id_set.hpp"
#include "interfaces/i_random_access.hpp"

struct i_ra_rule_id_set : i_rule_id_set, i_random_access<rule_id> {
    virtual ~i_ra_rule_id_set() = default;
};

#endif
