#include <memory>
#include "infrastructure/db_rule_set_factory.hpp"
#include "infrastructure/rule_id_set.hpp"

std::unique_ptr<i_rule_id_set> db_rule_set_factory::make() const {
    return std::make_unique<rule_id_set>();
}
