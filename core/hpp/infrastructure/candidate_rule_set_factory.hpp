#ifndef CANDIDATE_RULE_SET_FACTORY_HPP
#define CANDIDATE_RULE_SET_FACTORY_HPP

#include "interfaces/i_candidate_rule_set_factory.hpp"

struct candidate_rule_set_factory : i_candidate_rule_set_factory {
    std::unique_ptr<i_rule_id_set> make() const override;
};

#endif
