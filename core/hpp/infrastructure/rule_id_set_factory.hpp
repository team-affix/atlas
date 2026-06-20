#ifndef RULE_ID_SET_FACTORY_HPP
#define RULE_ID_SET_FACTORY_HPP

#include "infrastructure/rule_id_set.hpp"

struct rule_id_set_factory {
    rule_id_set make() const;
};

inline rule_id_set rule_id_set_factory::make() const {
    return rule_id_set{};
}

#endif
