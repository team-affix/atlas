#ifndef RA_RULE_ID_SET_HPP
#define RA_RULE_ID_SET_HPP

#include <unordered_map>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

struct ra_rule_id_set {
    void insert(rule_id);
    void erase(rule_id);
    bool contains(rule_id) const;
    coroutine<rule_id, void> iterate() const;
    rule_id front() const;
    size_t size() const;
    ra_rule_id_set copy() const;
    rule_id select(size_t index) const;
private:
    std::unordered_map<rule_id, size_t> index_;
    std::vector<rule_id> items_;
};

inline void ra_rule_id_set::insert(rule_id r) {
    auto [_, inserted] = index_.emplace(r, items_.size());
    DEBUG_ASSERT(inserted);
    items_.push_back(r);
}

inline void ra_rule_id_set::erase(rule_id r) {
    auto it = index_.find(r);
    DEBUG_ASSERT(it != index_.end());
    size_t idx = it->second;
    rule_id back = items_.back();
    items_.at(idx) = back;
    index_.at(back) = idx;
    items_.pop_back();
    index_.erase(it);
}

inline bool ra_rule_id_set::contains(rule_id r) const {
    return index_.contains(r);
}

inline coroutine<rule_id, void> ra_rule_id_set::iterate() const {
    for (rule_id r : items_)
        co_yield r;
}

inline rule_id ra_rule_id_set::front() const {
    return items_.at(0);
}

inline size_t ra_rule_id_set::size() const {
    return items_.size();
}

inline ra_rule_id_set ra_rule_id_set::copy() const {
    ra_rule_id_set snapshot;
    snapshot.items_ = items_;
    snapshot.index_ = index_;
    return snapshot;
}

inline rule_id ra_rule_id_set::select(size_t index) const {
    return items_.at(index);
}

#endif
