#ifndef PUD_CANDIDATE_REBASER_HPP
#define PUD_CANDIDATE_REBASER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include "value_objects/expr.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_pair.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "debug_assert.hpp"

template<typename IEnterPath,
         typename IResumeWitnessSearch,
         typename IResumeCandidateSearch,
         typename IGetParent>
struct pud_candidate_rebaser {
    pud_candidate_rebaser(IEnterPath& enter_path,
                          IResumeWitnessSearch& resume_witness_search,
                          IResumeCandidateSearch& resume_candidate_search,
                          IGetParent& get_parent);
    std::optional<pud_candidate_search_context> rebase(
        const pud_rule_id* query_leaf,
        size_t body_goal_idx,
        const expr* body_goal,
        uint32_t frame_offset,
        const pud_rule_id* cursor,
        const std::optional<pud_witness_pair>& pins,
        const std::vector<const expr*>& added_body_goals);
private:
    const pud_rule_id* failed_child(const pud_rule_id* last,
                                    const pud_rule_id* dest) const;
    void retarget_side(pud_witness_search_context& side,
                       const pud_rule_id* query_leaf,
                       size_t body_goal_idx,
                       const expr* body_goal,
                       uint32_t frame_offset) const;

    IEnterPath& enter_path_;
    IResumeWitnessSearch& resume_witness_search_;
    IResumeCandidateSearch& resume_candidate_search_;
    IGetParent& get_parent_;
};

template<typename IEP, typename IRWS, typename IRCS, typename IGP>
pud_candidate_rebaser<IEP, IRWS, IRCS, IGP>::pud_candidate_rebaser(
        IEP& enter_path,
        IRWS& resume_witness_search,
        IRCS& resume_candidate_search,
        IGP& get_parent)
    : enter_path_(enter_path)
    , resume_witness_search_(resume_witness_search)
    , resume_candidate_search_(resume_candidate_search)
    , get_parent_(get_parent) {}

template<typename IEP, typename IRWS, typename IRCS, typename IGP>
void pud_candidate_rebaser<IEP, IRWS, IRCS, IGP>::retarget_side(
        pud_witness_search_context& side,
        const pud_rule_id* query_leaf,
        size_t body_goal_idx,
        const expr* body_goal,
        uint32_t frame_offset) const {
    side.query_leaf = query_leaf;
    side.body_goal_idx = body_goal_idx;
    side.body_goal = body_goal;
    side.frame_offset = frame_offset;
}

template<typename IEP, typename IRWS, typename IRCS, typename IGP>
const pud_rule_id* pud_candidate_rebaser<IEP, IRWS, IRCS, IGP>::failed_child(
        const pud_rule_id* last,
        const pud_rule_id* dest) const {
    DEBUG_ASSERT(dest != nullptr);
    DEBUG_ASSERT(last != dest);
    const pud_rule_id* walk = dest;
    while (walk != nullptr) {
        const pud_rule_id* parent = get_parent_.get(walk);
        if (parent == last)
            return walk;
        walk = parent;
    }
    DEBUG_ASSERT(false);
    return dest;
}

template<typename IEP, typename IRWS, typename IRCS, typename IGP>
std::optional<pud_candidate_search_context>
pud_candidate_rebaser<IEP, IRWS, IRCS, IGP>::rebase(
        const pud_rule_id* query_leaf,
        size_t body_goal_idx,
        const expr* body_goal,
        uint32_t frame_offset,
        const pud_rule_id* cursor,
        const std::optional<pud_witness_pair>& pins,
        const std::vector<const expr*>& added_body_goals) {
    if (cursor == nullptr)
        return std::nullopt;
    const pud_rule_id* spine = enter_path_.enter_to(
        query_leaf, body_goal_idx, frame_offset, cursor);
    if (spine != cursor)
        return std::nullopt;
    std::optional<pud_witness_pair> witnesses = pins;
    if (witnesses.has_value()) {
        retarget_side(witnesses->a, query_leaf, body_goal_idx, body_goal, frame_offset);
        retarget_side(witnesses->b, query_leaf, body_goal_idx, body_goal, frame_offset);
        if (witnesses->a.current != nullptr) {
            const pud_rule_id* last = enter_path_.enter_to(
                query_leaf, body_goal_idx, frame_offset, witnesses->a.current);
            DEBUG_ASSERT(last != nullptr);
            if (last != witnesses->a.current)
                witnesses->a.current = failed_child(last, witnesses->a.current);
        }
        if (witnesses->b.current != nullptr) {
            const pud_rule_id* last = enter_path_.enter_to(
                query_leaf, body_goal_idx, frame_offset, witnesses->b.current);
            DEBUG_ASSERT(last != nullptr);
            if (last != witnesses->b.current)
                witnesses->b.current = failed_child(last, witnesses->b.current);
        }
    }
    pud_candidate_search_context context{
        query_leaf,
        body_goal_idx,
        body_goal,
        frame_offset,
        cursor,
        std::move(witnesses),
        added_body_goals};
    if (context.witnesses.has_value()) {
        resume_witness_search_.resume(context.witnesses->a);
        resume_witness_search_.resume(context.witnesses->b);
    }
    resume_candidate_search_.resume(context);
    if (context.cursor == nullptr)
        return std::nullopt;
    if (context.witnesses.has_value()) {
        if (context.witnesses->a.current == nullptr
                || context.witnesses->b.current == nullptr)
            return std::nullopt;
    }
    return context;
}

#endif
