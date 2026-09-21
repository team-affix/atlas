#ifndef PUD_NODE_ADDED_BODY_GOALS_HPP
#define PUD_NODE_ADDED_BODY_GOALS_HPP

#include <unordered_map>
#include <utility>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "debug_assert.hpp"

struct pud_node_added_body_goals {
    pud_node_added_body_goals();
    void store(const pud_rule_id* id, std::vector<const expr*> added_body_goals);
    const std::vector<const expr*>& get(const pud_rule_id* id) const;
private:
    using map_t = std::unordered_map<const pud_rule_id*, std::vector<const expr*>>;

    map_t by_id_;
};

#endif
