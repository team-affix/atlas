#ifndef RULE_UNFOLDER_HPP
#define RULE_UNFOLDER_HPP

#include <unordered_map>
#include <vector>
#include "infrastructure/bind_map.hpp"
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
    bool try_unify(const expr* goal_skeleton, const expr* orig_head, uint32_t orig_frame,
                   bind_map<IGlobalize>& bm);
    const expr* apply(
        framed_expr fe,
        bind_map<IGlobalize>& bm,
        std::unordered_map<uint32_t, uint32_t>& global_to_local,
        uint32_t& next_local);
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
    const expr* goal_skeleton, const expr* orig_head, uint32_t orig_frame,
    bind_map<IG>& bm) {
    unifier<IG, bind_map<IG>> u{globalize_, &bm};
    auto task = u.unify({goal_skeleton, 0}, {orig_head, orig_frame});
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
    const expr* goal_skeleton = subject->body[body_idx];
    const uint32_t orig_frame = subject->var_count;

    std::vector<rule_id> new_ids;
    auto candidate_iter = get_candidate_rules_.get_candidate_rules({goal_skeleton, 0}).iterate();
    while (!candidate_iter.done()) {
        candidate_iter.resume();
        if (!candidate_iter.has_yield()) continue;
        const rule_id orig_id = candidate_iter.consume_yield();
        const rule* orig = get_original_.get_rule(orig_id);

        bind_map<IG> bm{globalize_};
        if (!try_unify(goal_skeleton, orig->head, orig_frame, bm)) continue;

        std::unordered_map<uint32_t, uint32_t> global_to_local;
        uint32_t next_local = 0;

        const expr* new_head = apply({subject->head, 0}, bm, global_to_local, next_local);
        std::vector<const expr*> new_body;
        for (subgoal_id i = 0; i < body_idx; ++i)
            new_body.push_back(apply({subject->body[i], 0}, bm, global_to_local, next_local));
        for (const expr* orig_goal : orig->body)
            new_body.push_back(apply({orig_goal, orig_frame}, bm, global_to_local, next_local));
        for (subgoal_id i = body_idx + 1; i < subject->body.size(); ++i)
            new_body.push_back(apply({subject->body[i], 0}, bm, global_to_local, next_local));

        new_ids.push_back(push_expanded_.push(rule{new_head, std::move(new_body), next_local}));
    }

    erase_expanded_.erase(subject_id);
    return new_ids;
}

template<typename IGER, typename IEER, typename IPER, typename IGCR, typename IGOR, typename IMF, typename IMV, typename IG>
const expr* rule_unfolder<IGER,IEER,IPER,IGCR,IGOR,IMF,IMV,IG>::apply(
    framed_expr fe,
    bind_map<IG>& bm,
    std::unordered_map<uint32_t, uint32_t>& global_to_local,
    uint32_t& next_local) {
    const framed_expr resolved = bm.whnf(fe);
    if (const expr::var* v = std::get_if<expr::var>(&resolved.skeleton->content)) {
        const uint32_t global_key = globalize_.globalize(resolved.frame_offset, v->index);
        auto [it, inserted] = global_to_local.emplace(global_key, next_local);
        if (inserted) ++next_local;
        return make_var_.make_var(it->second);
    }
    const expr::functor& f = std::get<expr::functor>(resolved.skeleton->content);
    std::vector<const expr*> args;
    args.reserve(f.args.size());
    for (const expr* arg : f.args)
        args.push_back(apply({arg, resolved.frame_offset}, bm, global_to_local, next_local));
    return make_functor_.make_functor(f.id, args);
}

#endif
