#ifndef RULE_UNFOLDER_HPP
#define RULE_UNFOLDER_HPP

#include <unordered_map>
#include <vector>
#include "infrastructure/bind_map.hpp"
#include "infrastructure/globalizer.hpp"
#include "infrastructure/rule_id_set.hpp"
#include "infrastructure/unifier.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<
    typename IGetExpandedRule,
    typename IEraseExpandedRule,
    typename IPushExpandedRule,
    typename ILookupAllOriginalRules,
    typename ILookupOriginalByFunctor,
    typename IGetOriginalRule,
    typename IMakeFunctor,
    typename IMakeVar>
struct rule_unfolder {
    rule_unfolder(
        IGetExpandedRule&,
        IEraseExpandedRule&,
        IPushExpandedRule&,
        ILookupAllOriginalRules&,
        ILookupOriginalByFunctor&,
        IGetOriginalRule&,
        IMakeFunctor&,
        IMakeVar&);
    std::vector<rule_id> unfold(rule_id subject_id, subgoal_id body_idx);
private:
    const expr* apply(
        framed_expr fe,
        bind_map<globalizer>& bm,
        globalizer& g,
        std::unordered_map<uint32_t, uint32_t>& global_to_local,
        uint32_t& next_local);
    IGetExpandedRule&         get_expanded_;
    IEraseExpandedRule&       erase_expanded_;
    IPushExpandedRule&        push_expanded_;
    ILookupAllOriginalRules&  lookup_all_original_;
    ILookupOriginalByFunctor& lookup_original_by_functor_;
    IGetOriginalRule&         get_original_;
    IMakeFunctor&             make_functor_;
    IMakeVar&                 make_var_;
};

template<typename IGER, typename IEER, typename IPER, typename ILAOR, typename ILOBF, typename IGOR, typename IMF, typename IMV>
rule_unfolder<IGER,IEER,IPER,ILAOR,ILOBF,IGOR,IMF,IMV>::rule_unfolder(
    IGER& get_expanded,
    IEER& erase_expanded,
    IPER& push_expanded,
    ILAOR& lookup_all_original,
    ILOBF& lookup_original_by_functor,
    IGOR& get_original,
    IMF& make_functor,
    IMV& make_var)
    : get_expanded_(get_expanded)
    , erase_expanded_(erase_expanded)
    , push_expanded_(push_expanded)
    , lookup_all_original_(lookup_all_original)
    , lookup_original_by_functor_(lookup_original_by_functor)
    , get_original_(get_original)
    , make_functor_(make_functor)
    , make_var_(make_var) {}

template<typename IGER, typename IEER, typename IPER, typename ILAOR, typename ILOBF, typename IGOR, typename IMF, typename IMV>
std::vector<rule_id> rule_unfolder<IGER,IEER,IPER,ILAOR,ILOBF,IGOR,IMF,IMV>::unfold(
    rule_id subject_id, subgoal_id body_idx) {
    const rule* subject = get_expanded_.get_rule(subject_id);
    const expr* goal_skeleton = subject->body[body_idx];
    const uint32_t orig_frame = subject->var_count;

    const rule_id_set* candidates = nullptr;
    if (std::holds_alternative<expr::functor>(goal_skeleton->content)) {
        const uint32_t functor_id = std::get<expr::functor>(goal_skeleton->content).id;
        candidates = &lookup_original_by_functor_.lookup_rule_by_outermost_functor(functor_id);
    } else {
        candidates = &lookup_all_original_.lookup_all_rules();
    }

    std::vector<rule_id> new_ids;
    auto candidate_iter = candidates->iterate();
    while (!candidate_iter.done()) {
        candidate_iter.resume();
        if (!candidate_iter.has_yield())
            continue;
        const rule_id orig_id = candidate_iter.consume_yield();
        const rule* orig = get_original_.get_rule(orig_id);

        globalizer g;
        bind_map<globalizer> bm{g};
        unifier<globalizer, bind_map<globalizer>> u{g, &bm};

        auto unify_task = u.unify({goal_skeleton, 0}, {orig->head, orig_frame});
        while (!unify_task.done()) {
            unify_task.resume();
            if (unify_task.has_yield())
                unify_task.consume_yield();
        }
        if (!unify_task.result())
            continue;

        std::unordered_map<uint32_t, uint32_t> global_to_local;
        uint32_t next_local = 0;

        const expr* new_head = apply({subject->head, 0}, bm, g, global_to_local, next_local);

        std::vector<const expr*> new_body;
        for (subgoal_id i = 0; i < body_idx; ++i)
            new_body.push_back(apply({subject->body[i], 0}, bm, g, global_to_local, next_local));
        for (const expr* orig_goal : orig->body)
            new_body.push_back(apply({orig_goal, orig_frame}, bm, g, global_to_local, next_local));
        for (subgoal_id i = body_idx + 1; i < subject->body.size(); ++i)
            new_body.push_back(apply({subject->body[i], 0}, bm, g, global_to_local, next_local));

        const rule_id new_id = push_expanded_.push(rule{new_head, std::move(new_body), next_local});
        new_ids.push_back(new_id);
    }

    erase_expanded_.erase(subject_id);
    return new_ids;
}

template<typename IGER, typename IEER, typename IPER, typename ILAOR, typename ILOBF, typename IGOR, typename IMF, typename IMV>
const expr* rule_unfolder<IGER,IEER,IPER,ILAOR,ILOBF,IGOR,IMF,IMV>::apply(
    framed_expr fe,
    bind_map<globalizer>& bm,
    globalizer& g,
    std::unordered_map<uint32_t, uint32_t>& global_to_local,
    uint32_t& next_local) {
    const framed_expr resolved = bm.whnf(fe);
    if (const expr::var* v = std::get_if<expr::var>(&resolved.skeleton->content)) {
        const uint32_t global_key = g.globalize(resolved.frame_offset, v->index);
        auto [it, inserted] = global_to_local.emplace(global_key, next_local);
        if (inserted)
            ++next_local;
        return make_var_.make_var(it->second);
    }
    const expr::functor& f = std::get<expr::functor>(resolved.skeleton->content);
    std::vector<const expr*> args;
    args.reserve(f.args.size());
    for (const expr* arg : f.args)
        args.push_back(apply({arg, resolved.frame_offset}, bm, g, global_to_local, next_local));
    return make_functor_.make_functor(f.id, args);
}

#endif
