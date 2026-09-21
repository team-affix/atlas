#ifndef PUD_NODE_ADDED_UNIFICATIONS_HPP
#define PUD_NODE_ADDED_UNIFICATIONS_HPP

#include <unordered_map>
#include <utility>
#include <vector>
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

struct pud_node_added_unifications {
    pud_node_added_unifications();
    void store(const pud_rule_id* id, std::vector<pud_added_unification> added_unifications);
    const std::vector<pud_added_unification>& get(const pud_rule_id* id) const;
private:
    using map_t = std::unordered_map<const pud_rule_id*, std::vector<pud_added_unification>>;

    map_t by_id_;
};

#endif
