#ifndef PUD_CANDIDATE_SEARCH_HPP
#define PUD_CANDIDATE_SEARCH_HPP

#include <cstddef>
#include <optional>
#include <set>
#include <variant>
#include <vector>
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_candidate_search_result.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "value_objects/pud_witness_search_result.hpp"
#include "debug_assert.hpp"

template<typename IResumeWitnessSearch,
         typename IGetChildren,
         typename IGetParent,
         typename IUnifyHead,
         typename IGetAddedBodyGoals>
struct pud_candidate_search {
    pud_candidate_search(IResumeWitnessSearch& resume_witness_search,
                         IGetChildren& get_children,
                         IGetParent& get_parent,
                         IUnifyHead& unify_head,
                         IGetAddedBodyGoals& get_added_body_goals);
    pud_candidate_search_result resume(pud_candidate_search_context& context);
private:
    bool already_has_edge(const pud_candidate_search_context& context,
                          const pud_rule_id* edge_root) const;
    void query_advance(pud_candidate_search_context& context);
    void fill_live_edges(pud_candidate_search_context& context,
                         const std::set<const pud_rule_id*>& children);

    IResumeWitnessSearch& resume_witness_search_;
    IGetChildren& get_children_;
    IGetParent& get_parent_;
    IUnifyHead& unify_head_;
    IGetAddedBodyGoals& get_added_body_goals_;
};

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::pud_candidate_search(
        IRWS& resume_witness_search,
        IGC& get_children,
        IGP& get_parent,
        IUH& unify_head,
        IGABG& get_added_body_goals)
    : resume_witness_search_(resume_witness_search)
    , get_children_(get_children)
    , get_parent_(get_parent)
    , unify_head_(unify_head)
    , get_added_body_goals_(get_added_body_goals) {}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
bool pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::already_has_edge(
        const pud_candidate_search_context& context,
        const pud_rule_id* edge_root) const {
    for (const pud_witness_search_context& edge : context.live_edges) {
        if (edge.edge_root == edge_root)
            return true;
    }
    return false;
}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
void pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::query_advance(
        pud_candidate_search_context& context) {
    DEBUG_ASSERT(context.live_edges.size() == 1);
    const pud_rule_id* next_cursor = context.live_edges[0].edge_root;
    DEBUG_ASSERT(next_cursor != context.cursor);
    DEBUG_ASSERT(std::holds_alternative<pud_rule_id::inference>(next_cursor->content));
    const pud_rule_id::inference& inf = std::get<pud_rule_id::inference>(next_cursor->content);
    DEBUG_ASSERT(inf.call_site < context.added_body_goals.size());
    context.added_body_goals.erase(
        context.added_body_goals.begin() + static_cast<std::ptrdiff_t>(inf.call_site));
    const std::vector<const expr*>& next_goals = get_added_body_goals_.get(next_cursor);
    context.added_body_goals.insert(
        context.added_body_goals.end(),
        next_goals.begin(),
        next_goals.end());
    context.cursor = next_cursor;
    if (context.live_edges[0].current == context.cursor) {
        context.live_edges.clear();
        return;
    }
    const pud_rule_id* walk = context.live_edges[0].current;
    while (walk != context.cursor) {
        const pud_rule_id* parent = get_parent_.get(walk);
        DEBUG_ASSERT(parent != nullptr);
        if (parent == context.cursor) {
            context.live_edges[0].edge_root = walk;
            return;
        }
        walk = parent;
    }
}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
void pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::fill_live_edges(
        pud_candidate_search_context& context,
        const std::set<const pud_rule_id*>& children) {
    for (const pud_rule_id* child : children) {
        if (context.live_edges.size() >= 2)
            return;
        if (already_has_edge(context, child))
            continue;
        pud_witness_search_context edge{
            context.interval, context.body_goal, context.frame_offset, child, child};
        const pud_witness_search_result result = resume_witness_search_.resume(edge);
        if (std::holds_alternative<pud_witness_search_result::failed>(result.content))
            continue;
        context.live_edges.push_back(edge);
    }
}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
pud_candidate_search_result pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::resume(
        pud_candidate_search_context& context) {
    while (true) {
        if (context.live_edges.size() >= 2)
            return pud_candidate_search_result{pud_candidate_search_result::choice_point{}};
        const std::optional<std::set<const pud_rule_id*>> children =
            get_children_.get(context.cursor);
        if (!children.has_value()) {
            if (unify_head_.unify_head(context, context.cursor))
                return pud_candidate_search_result{pud_candidate_search_result::self_witness{}};
            return pud_candidate_search_result{pud_candidate_search_result::axiom_refuted{}};
        }

        fill_live_edges(context, *children);

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
