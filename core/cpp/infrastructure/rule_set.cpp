#include "../../hpp/infrastructure/rule_set.hpp"

void rule_set::insert(const rule* r) {
    rules_.insert(r);
}

void rule_set::erase(const rule* r) {
    rules_.erase(r);
}

state_machine<const rule*> rule_set::iterate() const {
    for (const rule* r : rules_)
        co_yield r;
}

size_t rule_set::size() const {
    return rules_.size();
}
