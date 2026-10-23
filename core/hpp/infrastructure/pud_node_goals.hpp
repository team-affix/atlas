#ifndef PUD_NODE_GOALS_HPP
#define PUD_NODE_GOALS_HPP

#include "infrastructure/pud_node_table.hpp"
#include "value_objects/expr.hpp"
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_goal_list.hpp"

// The open goals of every node, stored once when the node is indexed.
struct pud_node_goals {
    void store(pud_global_node_id id, pud_goal_list goals);
    const pud_goal_list& get(pud_global_node_id id) const;
    const expr* find(pud_global_node_id id, pud_body_goal_id goal) const;
private:
    pud_node_table<pud_goal_list> goals_;
};

#endif
