#ifndef I_DB_RULE_ID_SET_FACTORY_HPP
#define I_DB_RULE_ID_SET_FACTORY_HPP

#include "i_factory.hpp"
#include "i_rule_id_set.hpp"

struct i_db_rule_id_set_factory : i_factory<i_rule_id_set> {
    virtual ~i_db_rule_id_set_factory() = default;
};

#endif
