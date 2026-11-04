#ifndef PUD_TEST_NODE_QUERIES_HPP
#define PUD_TEST_NODE_QUERIES_HPP

#include <map>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_queries.hpp"

// The head each goal of a node's queries is owned for, so tests can compare query maps, whose owners are not
// comparable.
template<typename IRemoveQuery>
std::map<pud_body_goal_id, pud_mhq_head_id> heads_of(const pud_node_queries<IRemoveQuery>& queries) {
    std::map<pud_body_goal_id, pud_mhq_head_id> heads;
    for (const auto& [goal, owner] : queries)
        heads.emplace(goal, owner->head_id());
    return heads;
}

#endif
