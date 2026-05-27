#include "../../hpp/infrastructure/db.hpp"

db::db() = default;

db::db(rule_set total_rules)
    : total_rules_(std::move(total_rules)) {}

i_rule_set& db::get(const goal_lineage* /*gl*/) {
    return total_rules_;
}
