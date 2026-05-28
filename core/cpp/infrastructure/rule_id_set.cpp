#include "../../hpp/infrastructure/rule_id_set.hpp"

void rule_id_set::insert(rule_id r) {
    rule_ids_.insert(r);
}

void rule_id_set::erase(rule_id r) {
    rule_ids_.erase(r);
}

state_machine<rule_id> rule_id_set::iterate() const {
    for (rule_id r : rule_ids_)
        co_yield r;
}

size_t rule_id_set::size() const {
    return rule_ids_.size();
}
