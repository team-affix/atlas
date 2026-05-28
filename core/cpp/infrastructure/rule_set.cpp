#include "../../hpp/infrastructure/rule_set.hpp"

void rule_set::insert(rule_id r) {
    rules_.insert(r);
}

void rule_set::erase(rule_id r) {
    rules_.erase(r);
}

state_machine<rule_id> rule_set::iterate() const {
    for (rule_id r : rules_)
        co_yield r;
}

size_t rule_set::size() const {
    return rules_.size();
}
