#ifndef PUD_WITNESS_SEARCH_HPP
#define PUD_WITNESS_SEARCH_HPP

#include <vector>
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"
#include "debug_assert.hpp"

template<typename IIsLeaf, typename IOrderedChildren, typename ITryParent, typename IUnifyHead>
struct pud_witness_search {
    pud_witness_search(IIsLeaf& is_leaf,
                       IOrderedChildren& ordered_children,
                       ITryParent& try_parent,
                       IUnifyHead& unify_head);
    pud_witness_search_result resume(pud_query& query, pud_witness_search_context& context);
private:
    bool is_acceptable_witness(pud_query& query, const pud_rule_id* node) const;
    bool try_subtree(pud_query& query, pud_witness_search_context& context, const pud_rule_id* node);
    bool try_next_siblings(pud_query& query,
                           pud_witness_search_context& context,
                           const pud_rule_id* node);

    IIsLeaf& is_leaf_;
    IOrderedChildren& ordered_children_;
    ITryParent& try_parent_;
    IUnifyHead& unify_head_;
};

template<typename IIL, typename IOC, typename ITP, typename IUH>
pud_witness_search<IIL, IOC, ITP, IUH>::pud_witness_search(IIL& is_leaf,
                                                          IOC& ordered_children,
                                                          ITP& try_parent,
                                                          IUH& unify_head)
    : is_leaf_(is_leaf)
    , ordered_children_(ordered_children)
    , try_parent_(try_parent)
    , unify_head_(unify_head) {}

template<typename IIL, typename IOC, typename ITP, typename IUH>
bool pud_witness_search<IIL, IOC, ITP, IUH>::is_acceptable_witness(
        pud_query& query, const pud_rule_id* node) const {
    return is_leaf_.is_leaf(node) && unify_head_.unify_head(query, node);
}

template<typename IIL, typename IOC, typename ITP, typename IUH>
bool pud_witness_search<IIL, IOC, ITP, IUH>::try_subtree(
        pud_query& query, pud_witness_search_context& context, const pud_rule_id* node) {
    if (!unify_head_.unify_head(query, node))
        return false;
    if (is_leaf_.is_leaf(node)) {
        context.current = node;
        return true;
    }
    const std::vector<const pud_rule_id*> children = ordered_children_.ordered_children(node);
    for (const pud_rule_id* child : children) {
        if (try_subtree(query, context, child))
            return true;
    }
    return false;
}

template<typename IIL, typename IOC, typename ITP, typename IUH>
bool pud_witness_search<IIL, IOC, ITP, IUH>::try_next_siblings(
        pud_query& query, pud_witness_search_context& context, const pud_rule_id* node) {
    const pud_rule_id* walk = node;
    while (walk != context.edge_root) {
        const pud_rule_id* parent = try_parent_.try_parent(walk);
        DEBUG_ASSERT(parent != nullptr);
        const std::vector<const pud_rule_id*> siblings =
            ordered_children_.ordered_children(parent);
        bool past_walk = false;
        for (const pud_rule_id* sibling : siblings) {
            if (!past_walk) {
                if (sibling == walk)
                    past_walk = true;
                continue;
            }
            if (try_subtree(query, context, sibling))
                return true;
        }
        walk = parent;
    }
    return false;
}

template<typename IIL, typename IOC, typename ITP, typename IUH>
pud_witness_search_result pud_witness_search<IIL, IOC, ITP, IUH>::resume(
        pud_query& query, pud_witness_search_context& context) {
    if (is_acceptable_witness(query, context.current))
        return pud_witness_search_result{pud_witness_search_result::found{}};

    if (try_subtree(query, context, context.current))
        return pud_witness_search_result{pud_witness_search_result::found{}};

    if (try_next_siblings(query, context, context.current))
        return pud_witness_search_result{pud_witness_search_result::found{}};

    return pud_witness_search_result{pud_witness_search_result::failed{}};
}

#endif
