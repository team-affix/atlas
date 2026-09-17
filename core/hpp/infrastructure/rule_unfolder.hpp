#ifndef RULE_UNFOLDER_HPP
#define RULE_UNFOLDER_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>
#include "infrastructure/bind_map.hpp"
#include "infrastructure/normalizer.hpp"
#include "infrastructure/unifier.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<
    typename IGetExpandedRule,
    typename IEraseExpandedRule,
    typename IPushExpandedRule,
    typename IGetCandidateRules,
    typename IGetOriginalRule,
    typename IMakeFunctor,
    typename IMakeVar,
    typename IGlobalize>
struct rule_unfolder {

    rule_unfolder(
        IGetExpandedRule&,
        IEraseExpandedRule&,
        IPushExpandedRule&,
        IGetCandidateRules&,
        IGetOriginalRule&,
        IMakeFunctor&,
        IMakeVar&,
        IGlobalize&);

    std::vector<rule_id> unfold(rule_id subject_id, subgoal_id body_idx);

private:

    bool try_unify(framed_expr goal_fe, framed_expr head_fe, bind_map<IGlobalize>& bm);

    IGetExpandedRule&   get_expanded_;
    IEraseExpandedRule& erase_expanded_;
    IPushExpandedRule&  push_expanded_;
    IGetCandidateRules& get_candidate_rules_;
    IGetOriginalRule&   get_original_;
    IMakeFunctor&       make_functor_;
    IMakeVar&           make_var_;
    IGlobalize&         globalize_;
};

template<typename IGER, typename IEER, typename IPER, typename IGCR, typename IGOR, typename IMF, typename IMV, typename IG>
rule_unfolder<IGER,IEER,IPER,IGCR,IGOR,IMF,IMV,IG>::rule_unfolder(
    IGER& get_expanded,
    IEER& erase_expanded,
    IPER& push_expanded,
    IGCR& get_candidate_rules,
    IGOR& get_original,
    IMF& make_functor,
    IMV& make_var,
    IG& globalize)
    : get_expanded_(get_expanded)
    , erase_expanded_(erase_expanded)
    , push_expanded_(push_expanded)
    , get_candidate_rules_(get_candidate_rules)
    , get_original_(get_original)
    , make_functor_(make_functor)
    , make_var_(make_var)
    , globalize_(globalize) {}

template<typename IGER, typename IEER, typename IPER, typename IGCR, typename IGOR, typename IMF, typename IMV, typename IG>
bool rule_unfolder<IGER,IEER,IPER,IGCR,IGOR,IMF,IMV,IG>::try_unify(
    framed_expr goal_fe, framed_expr head_fe,
    bind_map<IG>& bm) {

    unifier<IG, bind_map<IG>> u{globalize_, &bm};

    auto task = u.unify(goal_fe, head_fe);

    while (!task.done()) {
        task.resume();
        if (task.has_yield()) task.consume_yield();
    }

    return task.result();
}

template<typename IGER, typename IEER, typename IPER, typename IGCR, typename IGOR, typename IMF, typename IMV, typename IG>
std::vector<rule_id> rule_unfolder<IGER,IEER,IPER,IGCR,IGOR,IMF,IMV,IG>::unfold(
    rule_id subject_id, subgoal_id body_idx) {

    const rule* subject = get_expanded_.get_rule(subject_id);

    const framed_expr goal_fe{subject->body.at(body_idx), 0};

    std::vector<rule_id> new_ids;

    auto candidate_iter = get_candidate_rules_.get_candidate_rules(goal_fe).iterate();

    while (!candidate_iter.done()) {

        candidate_iter.resume();
        if (!candidate_iter.has_yield()) continue;

        const rule_id base_id = candidate_iter.consume_yield();
        const rule* base      = get_original_.get_rule(base_id);

        // place the candidate rule's variables immediately after the subject's
        const framed_expr head_fe{base->head, subject->var_count};

        bind_map<IG> bm{globalize_};

        if (!try_unify(goal_fe, head_fe, bm)) continue;

        normalizer norm{globalize_, make_functor_, make_var_, bm};
        std::unordered_map<uint32_t, uint32_t> translation;

        const expr* new_head = norm.normalize({subject->head, 0}, 0, translation);

        std::vector<const expr*> new_body;

        for (subgoal_id i = 0; i < body_idx; ++i)
            new_body.push_back(norm.normalize({subject->body.at(i), 0}, 0, translation));

        for (const expr* base_subgoal : base->body)
            new_body.push_back(norm.normalize({base_subgoal, head_fe.frame_offset}, 0, translation));

        for (subgoal_id i = body_idx + 1; i < subject->body.size(); ++i)
            new_body.push_back(norm.normalize({subject->body.at(i), 0}, 0, translation));

        const uint32_t var_count = static_cast<uint32_t>(translation.size());
        new_ids.push_back(push_expanded_.push(rule{new_head, std::move(new_body), var_count}));
    }

    erase_expanded_.erase(subject_id);

    return new_ids;
}

#endif
