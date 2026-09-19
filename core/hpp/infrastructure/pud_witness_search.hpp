#ifndef PUD_WITNESS_SEARCH_HPP
#define PUD_WITNESS_SEARCH_HPP

#include <vector>
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"

template<typename IIsLeaf, typename IOrderedChildren, typename IParent, typename IUnifyHead>
struct pud_witness_search {
    pud_witness_search(IIsLeaf& is_leaf,
                       IOrderedChildren& ordered_children,
                       IParent& parent,
                       IUnifyHead& unify_head);
    pud_witness_search_result resume(pud_witness_search_context& context);
private:
    bool is_acceptable_witness(const pud_rule_id* node) const;
    bool try_subtree(pud_witness_search_context& context, const pud_rule_id* node);
    bool try_next_siblings(pud_witness_search_context& context, const pud_rule_id* node);

    IIsLeaf& is_leaf_;
    IOrderedChildren& ordered_children_;
    IParent& parent_;
    IUnifyHead& unify_head_;
};

template<typename IIL, typename IOC, typename IP, typename IUH>
pud_witness_search<IIL, IOC, IP, IUH>::pud_witness_search(IIL& is_leaf,
                                                          IOC& ordered_children,
                                                          IP& parent,
                                                          IUH& unify_head)
    : is_leaf_(is_leaf)
    , ordered_children_(ordered_children)
    , parent_(parent)
    , unify_head_(unify_head) {}

template<typename IIL, typename IOC, typename IP, typename IUH>
bool pud_witness_search<IIL, IOC, IP, IUH>::is_acceptable_witness(
        const pud_rule_id* node) const {
    return is_leaf_.is_leaf(node) && unify_head_.unify_head(node);
}

template<typename IIL, typename IOC, typename IP, typename IUH>
bool pud_witness_search<IIL, IOC, IP, IUH>::try_subtree(
        pud_witness_search_context& context, const pud_rule_id* node) {
    if (!unify_head_.unify_head(node))
        return false;
    if (is_leaf_.is_leaf(node)) {
        context.current = node;
        return true;
    }
    const std::vector<const pud_rule_id*> children = ordered_children_.ordered_children(node);
    for (const pud_rule_id* child : children) {
        if (try_subtree(context, child))
            return true;
    }
    return false;
}

template<typename IIL, typename IOC, typename IP, typename IUH>
bool pud_witness_search<IIL, IOC, IP, IUH>::try_next_siblings(
        pud_witness_search_context& context, const pud_rule_id* node) {
    const pud_rule_id* walk = node;
    while (walk != context.edge_root) {
        const pud_rule_id* parent = parent_.parent(walk);
        const std::vector<const pud_rule_id*> siblings =
            ordered_children_.ordered_children(parent);
        bool past_walk = false;
        for (const pud_rule_id* sibling : siblings) {
            if (!past_walk) {
                if (sibling == walk)
                    past_walk = true;
                continue;
            }
            if (try_subtree(context, sibling))
                return true;
        }
        walk = parent;
    }
    return false;
}

template<typename IIL, typename IOC, typename IP, typename IUH>
pud_witness_search_result pud_witness_search<IIL, IOC, IP, IUH>::resume(
        pud_witness_search_context& context) {
    if (is_acceptable_witness(context.current))
        return pud_witness_search_result{pud_witness_search_result::found{}};

    if (try_subtree(context, context.current))
        return pud_witness_search_result{pud_witness_search_result::found{}};

    if (try_next_siblings(context, context.current))
        return pud_witness_search_result{pud_witness_search_result::found{}};

    return pud_witness_search_result{pud_witness_search_result::failed{}};
}

#endif
