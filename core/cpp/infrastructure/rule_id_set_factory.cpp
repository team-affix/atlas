#include <memory>
#include "infrastructure/rule_id_set_factory.hpp"
#include "infrastructure/rule_id_set.hpp"

std::unique_ptr<i_rule_id_set> rule_id_set_factory::make() const {
    return std::make_unique<rule_id_set>();
}
