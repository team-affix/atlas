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

#endif
