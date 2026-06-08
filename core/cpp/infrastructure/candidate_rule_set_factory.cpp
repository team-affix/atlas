#include <memory>
#include "infrastructure/candidate_rule_set_factory.hpp"
#include "infrastructure/ra_rule_id_set.hpp"

std::unique_ptr<i_rule_id_set> candidate_rule_set_factory::make() const {
    return std::make_unique<ra_rule_id_set>();
}
