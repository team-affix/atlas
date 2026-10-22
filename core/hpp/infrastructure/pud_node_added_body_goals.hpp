#ifndef PUD_NODE_ADDED_BODY_GOALS_HPP
#define PUD_NODE_ADDED_BODY_GOALS_HPP

#include <unordered_map>
#include <vector>
#include "value_objects/pud_node_id.hpp"
#include "value_objects/expr.hpp"

struct pud_node_added_body_goals {
    const std::vector<const expr*>& get(pud_node_id id) const;
    void store(pud_node_id id, std::vector<const expr*> goals);
private:
    std::unordered_map<pud_node_id, std::vector<const expr*>> goals_;
};

#endif
