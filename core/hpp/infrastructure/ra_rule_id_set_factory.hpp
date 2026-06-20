#ifndef RA_RULE_ID_SET_FACTORY_HPP
#define RA_RULE_ID_SET_FACTORY_HPP

#include "infrastructure/ra_rule_id_set.hpp"

struct ra_rule_id_set_factory {
    ra_rule_id_set make() const;
};

inline ra_rule_id_set ra_rule_id_set_factory::make() const {
    return ra_rule_id_set{};
}

#endif
