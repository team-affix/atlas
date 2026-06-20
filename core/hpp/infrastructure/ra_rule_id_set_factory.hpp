#ifndef RA_RULE_ID_SET_FACTORY_HPP
#define RA_RULE_ID_SET_FACTORY_HPP

#include "infrastructure/ra_rule_id_set.hpp"

struct ra_rule_id_set_factory {
    ra_rule_id_set make() const;
};

#endif
