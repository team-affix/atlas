#ifndef I_CANDIDATE_RULE_SET_FACTORY_HPP
#define I_CANDIDATE_RULE_SET_FACTORY_HPP

#include "i_factory.hpp"
#include "i_rule_id_set.hpp"

struct i_candidate_rule_set_factory : i_factory<i_rule_id_set> {
    virtual ~i_candidate_rule_set_factory() = default;
};

#endif
