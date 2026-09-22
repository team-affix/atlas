#ifndef PUD_CANDIDATE_SEARCH_HPP
#define PUD_CANDIDATE_SEARCH_HPP

#include <cstddef>
#include <optional>
#include <set>
#include <variant>
#include <vector>
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_pair.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "debug_assert.hpp"

template<typename IResumeWitnessSearch,
         typename IGetChildren,
         typename IGetParent,
         typename IGetAddedBodyGoals>
struct pud_candidate_search {
    pud_candidate_search(IResumeWitnessSearch& resume_witness_search,
                         IGetChildren& get_children,
                         IGetParent& get_parent,
                         IGetAddedBodyGoals& get_added_body_goals);
    void resume(pud_candidate_search_context& context);
private:
    using children_set_t = std::set<const pud_rule_id*>;

    pud_witness_search_context make_edge(
        const pud_candidate_search_context& context,
        const pud_rule_id* search_root,
        const pud_rule_id* current) const;
    void advance(pud_candidate_search_context& context);

    IResumeWitnessSearch& resume_witness_search_;
    IGetChildren& get_children_;
    IGetParent& get_parent_;
    IGetAddedBodyGoals& get_added_body_goals_;
};

template<typename IRWS, typename IGC, typename IGP, typename IGABG>
pud_candidate_search<IRWS, IGC, IGP, IGABG>::pud_candidate_search(
        IRWS& resume_witness_search,
        IGC& get_children,
        IGP& get_parent,
        IGABG& get_added_body_goals)
    : resume_witness_search_(resume_witness_search)
    , get_children_(get_children)
    , get_parent_(get_parent)
    , get_added_body_goals_(get_added_body_goals) {}

template<typename IRWS, typename IGC, typename IGP, typename IGABG>
pud_witness_search_context
pud_candidate_search<IRWS, IGC, IGP, IGABG>::make_edge(
        const pud_candidate_search_context& context,
        const pud_rule_id* search_root,
        const pud_rule_id* current) const {
    return pud_witness_search_context{
        context.query_leaf,
        context.body_goal_idx,
        context.body_goal,
        context.frame_offset,
        search_root,
        current};
}

template<typename IRWS, typename IGC, typename IGP, typename IGABG>
void pud_candidate_search<IRWS, IGC, IGP, IGABG>::advance(
        pud_candidate_search_context& context) {
    DEBUG_ASSERT(context.witnesses.has_value());
    pud_witness_pair& pair = *context.witnesses;
    const bool a_live = pair.a.current != nullptr;
    const bool b_live = pair.b.current != nullptr;
    DEBUG_ASSERT(a_live != b_live);
    pud_witness_search_context& survivor = a_live ? pair.a : pair.b;
    pud_witness_search_context& dead = a_live ? pair.b : pair.a;
    const pud_rule_id* next_cursor = survivor.search_root;
    DEBUG_ASSERT(next_cursor != nullptr);
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
    dead.search_root = nullptr;
    dead.current = nullptr;
    if (survivor.current == context.cursor) {
        context.witnesses.reset();
        return;
    }
    const pud_rule_id* walk = survivor.current;
    while (walk != context.cursor) {
        const pud_rule_id* parent = get_parent_.get(walk);
        DEBUG_ASSERT(parent != nullptr);
        if (parent == context.cursor) {
            survivor.search_root = walk;
            return;
        }
        walk = parent;
    }
}

template<typename IRWS, typename IGC, typename IGP, typename IGABG>
void pud_candidate_search<IRWS, IGC, IGP, IGABG>::resume(
        pud_candidate_search_context& context) {
    while (true) {
        if (context.cursor == nullptr)
            return;
        if (context.witnesses.has_value()
                && context.witnesses->a.current != nullptr
                && context.witnesses->b.current != nullptr)
            return;

        const std::optional<children_set_t> children = get_children_.get(context.cursor);
        if (!children.has_value()) {
            context.witnesses.reset();
            pud_witness_search_context self =
                make_edge(context, context.cursor, context.cursor);
            resume_witness_search_.resume(self);
            if (self.current == nullptr)
                context.cursor = nullptr;
            return;
        }

        if (!context.witnesses.has_value())
            context.witnesses = pud_witness_pair{
                make_edge(context, nullptr, nullptr),
                make_edge(context, nullptr, nullptr)};
        pud_witness_pair& pair = *context.witnesses;
        const pud_rule_id* rightmost = nullptr;
        if (pair.a.search_root != nullptr)
            rightmost = pair.a.search_root;
        if (pair.b.search_root != nullptr) {
            if (rightmost == nullptr || pair.b.search_root > rightmost)
                rightmost = pair.b.search_root;
        }
        for (children_set_t::const_iterator it = rightmost == nullptr
                 ? children->begin()
                 : children->upper_bound(rightmost);
             it != children->end(); ++it) {
            if (pair.a.current != nullptr && pair.b.current != nullptr)
                return;
            pud_witness_search_context edge = make_edge(context, *it, *it);
            resume_witness_search_.resume(edge);
            if (edge.current == nullptr)
                continue;
            if (pair.a.current == nullptr) {
                pair.a = edge;
                continue;
            }
            pair.b = edge;
        }
        if (pair.a.current != nullptr && pair.b.current != nullptr)
            return;
        const bool one_live =
            (pair.a.current != nullptr) != (pair.b.current != nullptr);
        if (one_live) {
            advance(context);
            continue;
        }
        context.witnesses.reset();
        context.cursor = nullptr;
        return;
    }
}

#endif
