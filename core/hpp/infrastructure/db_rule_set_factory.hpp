#ifndef DB_RULE_SET_FACTORY_HPP
#define DB_RULE_SET_FACTORY_HPP

#include "interfaces/i_db_rule_set_factory.hpp"

struct db_rule_set_factory : i_db_rule_set_factory {
    std::unique_ptr<i_rule_id_set> make() const override;
};

#endif
