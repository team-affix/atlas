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

#endif
