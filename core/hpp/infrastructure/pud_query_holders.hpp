#ifndef PUD_QUERY_HOLDERS_HPP
#define PUD_QUERY_HOLDERS_HPP

#include <optional>
#include <set>
#include <unordered_map>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_unfold.hpp"

// Which live nodes hold each query. A query belongs to one goal id, the same at every node holding
// it, since a child inherits its parent's goal ids. A query lives exactly while some node holds it.
struct pud_query_holders {
    pud_query_holders();
    void hold(pud_mhq_head_id head_id, pud_global_node_id leaf, pud_body_goal_id goal);
    bool release(pud_mhq_head_id head_id, pud_global_node_id leaf);
    std::optional<pud_unfold> find_holder(pud_mhq_head_id head_id) const;
    std::vector<pud_global_node_id> get_holders(pud_mhq_head_id head_id) const;
private:
    struct holding {
        pud_body_goal_id goal;
        std::set<pud_global_node_id> leaves;
    };

    std::unordered_map<pud_mhq_head_id, holding> holdings_;
};

#endif
