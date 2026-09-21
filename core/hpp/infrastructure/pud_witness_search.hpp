#ifndef PUD_WITNESS_SEARCH_HPP
#define PUD_WITNESS_SEARCH_HPP

#include <optional>
#include <set>
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"
#include "debug_assert.hpp"

template<typename IGetChildren, typename IGetParent, typename IUnifyHead>
struct pud_witness_search {
    pud_witness_search(IGetChildren& get_children,
                       IGetParent& get_parent,
                       IUnifyHead& unify_head);
    pud_witness_search_result resume(pud_witness_search_context& context);
private:
    bool is_acceptable_witness(pud_witness_search_context& context,
                               const pud_rule_id* node) const;
    bool try_subtree(pud_witness_search_context& context, const pud_rule_id* node);
    bool try_next_siblings(pud_witness_search_context& context, const pud_rule_id* node);

    IGetChildren& get_children_;
    IGetParent& get_parent_;
    IUnifyHead& unify_head_;
};

template<typename IGC, typename IGP, typename IUH>
pud_witness_search<IGC, IGP, IUH>::pud_witness_search(IGC& get_children,
                                                     IGP& get_parent,
                                                     IUH& unify_head)
    : get_children_(get_children)
    , get_parent_(get_parent)
    , unify_head_(unify_head) {}

template<typename IGC, typename IGP, typename IUH>
bool pud_witness_search<IGC, IGP, IUH>::is_acceptable_witness(
        pud_witness_search_context& context, const pud_rule_id* node) const {
    return !get_children_.get(node).has_value() && unify_head_.unify_head(context, node);
}

template<typename IGC, typename IGP, typename IUH>
bool pud_witness_search<IGC, IGP, IUH>::try_subtree(
        pud_witness_search_context& context, const pud_rule_id* node) {
    if (!unify_head_.unify_head(context, node))
        return false;
    const std::optional<std::set<const pud_rule_id*>> children = get_children_.get(node);
    if (!children.has_value()) {
        context.current = node;
        return true;
    }
    for (const pud_rule_id* child : *children) {
        if (try_subtree(context, child))
            return true;
    }
    return false;
}

template<typename IGC, typename IGP, typename IUH>
bool pud_witness_search<IGC, IGP, IUH>::try_next_siblings(
        pud_witness_search_context& context, const pud_rule_id* node) {
    const pud_rule_id* walk = node;
    while (walk != context.edge_root) {
        const pud_rule_id* parent = get_parent_.get(walk);
        DEBUG_ASSERT(parent != nullptr);
        const std::optional<std::set<const pud_rule_id*>> siblings =
            get_children_.get(parent);
        DEBUG_ASSERT(siblings.has_value());
        bool past_walk = false;
        for (const pud_rule_id* sibling : *siblings) {
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

template<typename IGC, typename IGP, typename IUH>
pud_witness_search_result pud_witness_search<IGC, IGP, IUH>::resume(
        pud_witness_search_context& context) {
    if (is_acceptable_witness(context, context.current))
        return pud_witness_search_result{pud_witness_search_result::found{}};

    if (try_subtree(context, context.current))
        return pud_witness_search_result{pud_witness_search_result::found{}};

    if (try_next_siblings(context, context.current))
        return pud_witness_search_result{pud_witness_search_result::found{}};

    return pud_witness_search_result{pud_witness_search_result::failed{}};
}

#endif
