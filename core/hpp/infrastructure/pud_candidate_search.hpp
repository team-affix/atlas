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
         typename IUnifyHead,
         typename IGetAddedBodyGoals>
struct pud_candidate_search {
    pud_candidate_search(IResumeWitnessSearch& resume_witness_search,
                         IGetChildren& get_children,
                         IGetParent& get_parent,
                         IUnifyHead& unify_head,
                         IGetAddedBodyGoals& get_added_body_goals);
    void resume(pud_candidate_search_context& context);
private:
    using children_set_t = std::set<const pud_rule_id*>;

    bool both_live(const pud_witness_pair& pair) const;
    std::size_t live_count(const pud_witness_pair& pair) const;
    pud_witness_search_context& live_side(pud_witness_pair& pair);
    pud_witness_search_context& dead_side(pud_witness_pair& pair);
    children_set_t::const_iterator frontier(
        const children_set_t& children, const pud_witness_pair& pair) const;
    void try_fill(pud_candidate_search_context& context, const children_set_t& children);
    void advance(pud_candidate_search_context& context);
    void refute(pud_candidate_search_context& context);

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
bool pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::both_live(
        const pud_witness_pair& pair) const {
    return pair.a.current != nullptr && pair.b.current != nullptr;
}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
std::size_t pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::live_count(
        const pud_witness_pair& pair) const {
    std::size_t count = 0;
    if (pair.a.current != nullptr)
        ++count;
    if (pair.b.current != nullptr)
        ++count;
    return count;
}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
pud_witness_search_context&
pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::live_side(pud_witness_pair& pair) {
    if (pair.a.current != nullptr)
        return pair.a;
    return pair.b;
}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
pud_witness_search_context&
pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::dead_side(pud_witness_pair& pair) {
    if (pair.a.current == nullptr)
        return pair.a;
    return pair.b;
}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
typename pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::children_set_t::const_iterator
pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::frontier(
        const children_set_t& children, const pud_witness_pair& pair) const {
    const pud_rule_id* rightmost = nullptr;
    if (pair.a.search_root != nullptr)
        rightmost = pair.a.search_root;
    if (pair.b.search_root != nullptr) {
        if (rightmost == nullptr || pair.b.search_root > rightmost)
            rightmost = pair.b.search_root;
    }
    if (rightmost == nullptr)
        return children.begin();
    return children.upper_bound(rightmost);
}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
void pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::try_fill(
        pud_candidate_search_context& context, const children_set_t& children) {
    if (!context.witnesses.has_value()) {
        const pud_witness_search_context empty{
            context.interval, context.body_goal, context.frame_offset, nullptr, nullptr};
        context.witnesses = pud_witness_pair{empty, empty};
    }
    pud_witness_pair& pair = *context.witnesses;
    for (auto it = frontier(children, pair); it != children.end(); ++it) {
        if (both_live(pair))
            return;
        pud_witness_search_context edge{
            context.interval, context.body_goal, context.frame_offset, *it, *it};
        resume_witness_search_.resume(edge);
        if (edge.current == nullptr)
            continue;
        dead_side(pair) = edge;
    }
}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
void pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::advance(
        pud_candidate_search_context& context) {
    DEBUG_ASSERT(context.witnesses.has_value());
    pud_witness_pair& pair = *context.witnesses;
    DEBUG_ASSERT(live_count(pair) == 1);
    pud_witness_search_context& survivor = live_side(pair);
    pud_witness_search_context& dead = dead_side(pair);
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

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
void pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::refute(
        pud_candidate_search_context& context) {
    context.witnesses.reset();
    context.cursor = nullptr;
}

template<typename IRWS, typename IGC, typename IGP, typename IUH, typename IGABG>
void pud_candidate_search<IRWS, IGC, IGP, IUH, IGABG>::resume(
        pud_candidate_search_context& context) {
    while (true) {
        if (context.cursor == nullptr)
            return;
        if (context.witnesses.has_value() && both_live(*context.witnesses))
            return;

        const std::optional<children_set_t> children = get_children_.get(context.cursor);
        if (!children.has_value()) {
            context.witnesses.reset();
            if (!unify_head_.unify_head(context, context.cursor))
                context.cursor = nullptr;
            return;
        }

        try_fill(context, *children);
        if (both_live(*context.witnesses))
            return;
        if (live_count(*context.witnesses) == 1) {
            advance(context);
            continue;
        }
        refute(context);
        return;
    }
}

#endif
