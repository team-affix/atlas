#include <memory>
#include "infrastructure/ra_rule_id_set.hpp"
#include "debug_assert.hpp"

void ra_rule_id_set::insert(rule_id r) {
    auto [_, inserted] = index_.emplace(r, items_.size());
    DEBUG_ASSERT(inserted);
    items_.push_back(r);
}

void ra_rule_id_set::erase(rule_id r) {
    auto it = index_.find(r);
    DEBUG_ASSERT(it != index_.end());
    size_t idx = it->second;
    rule_id back = items_.back();
    items_.at(idx) = back;
    index_.at(back) = idx;
    items_.pop_back();
    index_.erase(it);
}

bool ra_rule_id_set::contains(rule_id r) const {
    return index_.contains(r);
}

coroutine<rule_id, void> ra_rule_id_set::iterate() const {
    for (rule_id r : items_)
        co_yield r;
}

rule_id ra_rule_id_set::front() const {
    return items_.at(0);
}

size_t ra_rule_id_set::size() const {
    return items_.size();
}

std::unique_ptr<i_rule_id_set> ra_rule_id_set::copy() const {
    auto snapshot = std::make_unique<ra_rule_id_set>();
    snapshot->items_ = items_;
    snapshot->index_ = index_;
    return snapshot;
}

rule_id ra_rule_id_set::select(size_t index) const {
    return items_.at(index);
}
