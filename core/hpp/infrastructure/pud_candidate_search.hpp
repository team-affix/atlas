#ifndef PUD_CANDIDATE_SEARCH_HPP
#define PUD_CANDIDATE_SEARCH_HPP

#include <variant>
#include <vector>
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_query.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"
#include "debug_assert.hpp"

template<typename IResumeWitnessSearch,
         typename IIsLeaf,
         typename IOrderedChildren,
         typename IParent,
         typename IUnifyHead>
struct pud_candidate_search {
    pud_candidate_search(IResumeWitnessSearch& resume_witness_search,
                         IIsLeaf& is_leaf,
                         IOrderedChildren& ordered_children,
                         IParent& parent,
                         IUnifyHead& unify_head);
    pud_candidate_search_result resume(pud_query& query, pud_candidate_search_context& context);
private:
    bool already_has_edge(const pud_candidate_search_context& context,
                          const pud_rule_id* edge_root) const;
    void query_advance(pud_candidate_search_context& context);
    void fill_live_edges(pud_query& query, pud_candidate_search_context& context);

    IResumeWitnessSearch& resume_witness_search_;
    IIsLeaf& is_leaf_;
    IOrderedChildren& ordered_children_;
    IParent& parent_;
    IUnifyHead& unify_head_;
};

template<typename IRWS, typename IIL, typename IOC, typename IP, typename IUH>
pud_candidate_search<IRWS, IIL, IOC, IP, IUH>::pud_candidate_search(
        IRWS& resume_witness_search,
        IIL& is_leaf,
        IOC& ordered_children,
        IP& parent,
        IUH& unify_head)
    : resume_witness_search_(resume_witness_search)
    , is_leaf_(is_leaf)
    , ordered_children_(ordered_children)
    , parent_(parent)
    , unify_head_(unify_head) {}

template<typename IRWS, typename IIL, typename IOC, typename IP, typename IUH>
bool pud_candidate_search<IRWS, IIL, IOC, IP, IUH>::already_has_edge(
        const pud_candidate_search_context& context,
        const pud_rule_id* edge_root) const {
    for (const pud_witness_search_context& edge : context.live_edges) {
        if (edge.edge_root == edge_root)
            return true;
    }
    return false;
}

template<typename IRWS, typename IIL, typename IOC, typename IP, typename IUH>
void pud_candidate_search<IRWS, IIL, IOC, IP, IUH>::query_advance(
        pud_candidate_search_context& context) {
    DEBUG_ASSERT(context.live_edges.size() == 1);
    const pud_rule_id* next_cursor = context.live_edges[0].edge_root;
    DEBUG_ASSERT(next_cursor != context.cursor);
    context.cursor = next_cursor;
    if (context.live_edges[0].current == context.cursor)
        return;
    const pud_rule_id* walk = context.live_edges[0].current;
    while (walk != context.cursor) {
        const pud_rule_id* parent = parent_.parent(walk);
        if (parent == context.cursor) {
            context.live_edges[0].edge_root = walk;
            return;
        }
        walk = parent;
    }
}

template<typename IRWS, typename IIL, typename IOC, typename IP, typename IUH>
void pud_candidate_search<IRWS, IIL, IOC, IP, IUH>::fill_live_edges(
        pud_query& query, pud_candidate_search_context& context) {
    const std::vector<const pud_rule_id*> children =
        ordered_children_.ordered_children(context.cursor);
    for (const pud_rule_id* child : children) {
        if (context.live_edges.size() >= 2)
            return;
        if (already_has_edge(context, child))
            continue;
        pud_witness_search_context edge{child, child};
        const pud_witness_search_result result = resume_witness_search_.resume(query, edge);
        if (std::holds_alternative<pud_witness_search_result::failed>(result.content))
            continue;
        context.live_edges.push_back(edge);
    }
}

template<typename IRWS, typename IIL, typename IOC, typename IP, typename IUH>
pud_candidate_search_result pud_candidate_search<IRWS, IIL, IOC, IP, IUH>::resume(
        pud_query& query, pud_candidate_search_context& context) {
    while (true) {
        if (context.live_edges.size() >= 2)
            return pud_candidate_search_result{pud_candidate_search_result::choice_point{}};
        if (is_leaf_.is_leaf(context.cursor) && unify_head_.unify_head(query, context.cursor))
            return pud_candidate_search_result{pud_candidate_search_result::self_witness{}};

        fill_live_edges(query, context);

        if (context.live_edges.size() >= 2)
            return pud_candidate_search_result{pud_candidate_search_result::choice_point{}};
        if (context.live_edges.size() == 1) {
            query_advance(context);
            continue;
        }
        return pud_candidate_search_result{pud_candidate_search_result::axiom_refuted{}};
    }
}

#endif
