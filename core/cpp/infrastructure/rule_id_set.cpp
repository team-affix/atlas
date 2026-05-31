#include <memory>
#include "infrastructure/rule_id_set.hpp"
#include "debug_assert.hpp"

void rule_id_set::insert(rule_id r) {
    auto [_, inserted] = rule_ids_.insert(r);
    DEBUG_ASSERT(inserted);
}

void rule_id_set::erase(rule_id r) {
    auto erased = rule_ids_.erase(r);
    DEBUG_ASSERT(erased == 1);
}

coroutine<rule_id, void> rule_id_set::iterate() const {
    for (rule_id r : rule_ids_)
        co_yield r;
}

size_t rule_id_set::size() const {
    return rule_ids_.size();
}

std::unique_ptr<i_rule_id_set> rule_id_set::copy() const {
    auto snapshot = std::make_unique<rule_id_set>();
    for (rule_id r : rule_ids_)
        snapshot->insert(r);
    return snapshot;
}
