#ifndef RA_RULE_ID_SET_FACTORY_HPP
#define RA_RULE_ID_SET_FACTORY_HPP

#include "interfaces/i_candidate_rule_id_set_factory.hpp"

struct ra_rule_id_set_factory : i_candidate_rule_id_set_factory {
    std::unique_ptr<i_rule_id_set> make() const override;
};

#endif
