#ifndef PUD_QUERY_LEAF_FINDER_HPP
#define PUD_QUERY_LEAF_FINDER_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_query_introduction.hpp"
#include "value_objects/pud_unfold.hpp"
#include "infrastructure/coroutine.hpp"

// Finds the live leaves that still hold a query, by walking down the tree from the node that introduced it. A
// leaf inherits the query unless a node on its path closed the goal or forked it. The walk skips refuted
// subtrees and children that closed the goal; a fork is seen at the leaf, whose queries then name another head.
// Each leaf is yielded as the unfold of the query's goal there, in depth-first order.
template<
    typename IFindQueryIntroduction,
    typename IGetChildren,
    typename IGetInference,
    typename IIsNodeRefuted,
    typename IFindNodeQueries>
struct pud_query_leaf_finder {
    pud_query_leaf_finder(
        IFindQueryIntroduction& find_query_introduction,
        IGetChildren& get_children,
        IGetInference& get_inference,
        IIsNodeRefuted& is_node_refuted,
        IFindNodeQueries& find_node_queries);
    coroutine<pud_unfold, void> find_leaves(pud_mhq_head_id head_id) const;
private:
    IFindQueryIntroduction& find_query_introduction_;
    IGetChildren& get_children_;
    IGetInference& get_inference_;
    IIsNodeRefuted& is_node_refuted_;
    IFindNodeQueries& find_node_queries_;
};

#define PUD_QUERY_LEAF_FINDER_TEMPLATE                                                                  \
    template<typename IFindQueryIntroduction, typename IGetChildren, typename IGetInference,            \
             typename IIsNodeRefuted, typename IFindNodeQueries>
#define PUD_QUERY_LEAF_FINDER                                                                           \
    pud_query_leaf_finder<IFindQueryIntroduction, IGetChildren, IGetInference, IIsNodeRefuted, IFindNodeQueries>

PUD_QUERY_LEAF_FINDER_TEMPLATE
PUD_QUERY_LEAF_FINDER::pud_query_leaf_finder(
    IFindQueryIntroduction& find_query_introduction,
    IGetChildren& get_children,
    IGetInference& get_inference,
    IIsNodeRefuted& is_node_refuted,
    IFindNodeQueries& find_node_queries)
    : find_query_introduction_(find_query_introduction)
    , get_children_(get_children)
    , get_inference_(get_inference)
    , is_node_refuted_(is_node_refuted)
    , find_node_queries_(find_node_queries) {}

PUD_QUERY_LEAF_FINDER_TEMPLATE
coroutine<pud_unfold, void> PUD_QUERY_LEAF_FINDER::find_leaves(pud_mhq_head_id head_id) const {
    const std::optional<pud_query_introduction> introduction = find_query_introduction_.find_introduction(head_id);
    if (!introduction.has_value())
        co_return;
    const pud_global_node_id introducer = introduction->node;
    const pud_body_goal_id goal = introduction->goal;
    std::vector<pud_global_node_id> pending{introducer};
    while (!pending.empty()) {
        const pud_global_node_id node = pending.back();
        pending.pop_back();
        // Only a live leaf keeps its queries; the nodes above it erased theirs when they were unfolded.
        const auto* queries = find_node_queries_.find(node);
        if (queries != nullptr) {
            const auto* owner = queries->find(goal);
            const bool holds = owner != nullptr && (*owner)->head_id() == head_id;
            if (holds)
                co_yield pud_unfold{.leaf = node, .goal = goal};
            continue;
        }
        const std::vector<pud_node_id>& children = get_children_.get_children(node);
        for (auto stored_child = children.rbegin(); stored_child != children.rend(); ++stored_child) {
            const pud_global_node_id child{.module = node.module, .node = *stored_child};
            if (is_node_refuted_.is_refuted(child))
                continue;
            if (get_inference_.get_inference(child).call_site == goal)
                continue;
            pending.push_back(child);
        }
    }
}

#undef PUD_QUERY_LEAF_FINDER
#undef PUD_QUERY_LEAF_FINDER_TEMPLATE

#endif
