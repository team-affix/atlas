#ifndef RULE_ID_SET_HPP
#define RULE_ID_SET_HPP

#include <unordered_set>
#include "infrastructure/coroutine.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

struct rule_id_set {
    void insert(rule_id);
    void erase(rule_id);
    bool contains(rule_id) const;
    coroutine<rule_id, void> iterate() const;
    rule_id front() const;
    size_t size() const;
    rule_id_set copy() const;
private:
    std::unordered_set<rule_id> rule_ids_;
};

inline void rule_id_set::insert(rule_id r) {
    auto [_, inserted] = rule_ids_.insert(r);
    DEBUG_ASSERT(inserted);
}

inline void rule_id_set::erase(rule_id r) {
    auto erased = rule_ids_.erase(r);
    DEBUG_ASSERT(erased == 1);
}

inline bool rule_id_set::contains(rule_id r) const {
    return rule_ids_.contains(r);
}

inline coroutine<rule_id, void> rule_id_set::iterate() const {
    for (rule_id r : rule_ids_)
        co_yield r;
}

inline rule_id rule_id_set::front() const {
    DEBUG_ASSERT(!rule_ids_.empty());
    return *rule_ids_.begin();
}

inline size_t rule_id_set::size() const {
    return rule_ids_.size();
}

inline rule_id_set rule_id_set::copy() const {
    rule_id_set snapshot;
    for (rule_id r : rule_ids_)
        snapshot.insert(r);
    return snapshot;
}

#endif
