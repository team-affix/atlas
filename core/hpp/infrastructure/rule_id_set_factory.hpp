#ifndef RULE_ID_SET_FACTORY_HPP
#define RULE_ID_SET_FACTORY_HPP

#include "interfaces/i_db_rule_set_factory.hpp"

struct rule_id_set_factory : i_db_rule_set_factory {
    std::unique_ptr<i_rule_id_set> make() const override;
};

#endif
