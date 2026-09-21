#ifndef PUD_UNFOLDER_HPP
#define PUD_UNFOLDER_HPP

#include <cstddef>
#include <cstdint>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_candidate_search_context.hpp"
#include "value_objects/pud_forced_unfold.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_unfold_site.hpp"
#include "debug_assert.hpp"

template<typename IUnfoldSite,
         typename ISetNormEnv,
         typename INormalize,
         typename IWhnf,
         typename IMakeVar,
         typename IGetLvc,
         typename IMakeInference,
         typename IStoreAddedUnifications,
         typename IGetAddedUnifications,
         typename IStoreAddedBodyGoals,
         typename IStoreLvc,
         typename IStoreChildren,
         typename IStoreParent,
         typename IGetParent,
         typename IGetInterval,
         typename IAllocateChildInterval,
         typename IStoreInterval,
         typename IRecordBinding,
         typename IGetAddedCallerReps,
         typename IReplaceUnfolded>
struct pud_unfolder {
    pud_unfolder(IUnfoldSite& unfold_site,
                 ISetNormEnv& set_norm_env,
                 INormalize& normalize,
                 IWhnf& whnf,
                 IMakeVar& make_var,
                 IGetLvc& get_lvc,
                 IMakeInference& make_inference,
                 IStoreAddedUnifications& store_added_unifications,
                 IGetAddedUnifications& get_added_unifications,
                 IStoreAddedBodyGoals& store_added_body_goals,
                 IStoreLvc& store_lvc,
                 IStoreChildren& store_children,
                 IStoreParent& store_parent,
                 IGetParent& get_parent,
                 IGetInterval& get_interval,
                 IAllocateChildInterval& allocate_child_interval,
                 IStoreInterval& store_interval,
                 IRecordBinding& record_binding,
                 IGetAddedCallerReps& get_added_caller_reps,
                 IReplaceUnfolded& replace_unfolded);
    coroutine<pud_forced_unfold, std::vector<const pud_rule_id*>> unfold(
        const pud_rule_id* leaf,
        size_t body_goal_idx);
private:
    const pud_rule_id* materialize_child(pud_candidate_search_context& ctx,
                                         const pud_rule_id* leaf,
                                         size_t body_goal_idx,
                                         uint32_t parent_lvc);

    IUnfoldSite& unfold_site_;
    ISetNormEnv& set_norm_env_;
    INormalize& normalize_;
    IWhnf& whnf_;
    IMakeVar& make_var_;
    IGetLvc& get_lvc_;
    IMakeInference& make_inference_;
    IStoreAddedUnifications& store_added_unifications_;
    IGetAddedUnifications& get_added_unifications_;
    IStoreAddedBodyGoals& store_added_body_goals_;
    IStoreLvc& store_lvc_;
    IStoreChildren& store_children_;
    IStoreParent& store_parent_;
    IGetParent& get_parent_;
    IGetInterval& get_interval_;
    IAllocateChildInterval& allocate_child_interval_;
    IStoreInterval& store_interval_;
    IRecordBinding& record_binding_;
    IGetAddedCallerReps& get_added_caller_reps_;
    IReplaceUnfolded& replace_unfolded_;
};

template<typename IUS, typename ISNE, typename IN, typename IW, typename IMV,
         typename IGL, typename IMI, typename ISAU, typename IGAU, typename ISABG, typename ISL,
         typename ISC, typename ISP, typename IGP, typename IGI, typename IACI,
         typename ISI, typename IRB, typename IGACR, typename IRU>
pud_unfolder<IUS, ISNE, IN, IW, IMV, IGL, IMI, ISAU, IGAU, ISABG, ISL, ISC, ISP, IGP,
             IGI, IACI, ISI, IRB, IGACR, IRU>::
pud_unfolder(IUS& unfold_site,
             ISNE& set_norm_env,
             IN& normalize,
             IW& whnf,
             IMV& make_var,
             IGL& get_lvc,
             IMI& make_inference,
             ISAU& store_added_unifications,
             IGAU& get_added_unifications,
             ISABG& store_added_body_goals,
             ISL& store_lvc,
             ISC& store_children,
             ISP& store_parent,
             IGP& get_parent,
             IGI& get_interval,
             IACI& allocate_child_interval,
             ISI& store_interval,
             IRB& record_binding,
             IGACR& get_added_caller_reps,
             IRU& replace_unfolded)
    : unfold_site_(unfold_site)
    , set_norm_env_(set_norm_env)
    , normalize_(normalize)
    , whnf_(whnf)
    , make_var_(make_var)
    , get_lvc_(get_lvc)
    , make_inference_(make_inference)
    , store_added_unifications_(store_added_unifications)
    , get_added_unifications_(get_added_unifications)
    , store_added_body_goals_(store_added_body_goals)
    , store_lvc_(store_lvc)
    , store_children_(store_children)
    , store_parent_(store_parent)
    , get_parent_(get_parent)
    , get_interval_(get_interval)
    , allocate_child_interval_(allocate_child_interval)
    , store_interval_(store_interval)
    , record_binding_(record_binding)
    , get_added_caller_reps_(get_added_caller_reps)
    , replace_unfolded_(replace_unfolded) {}

template<typename IUS, typename ISNE, typename IN, typename IW, typename IMV,
         typename IGL, typename IMI, typename ISAU, typename IGAU, typename ISABG, typename ISL,
         typename ISC, typename ISP, typename IGP, typename IGI, typename IACI,
         typename ISI, typename IRB, typename IGACR, typename IRU>
const pud_rule_id*
pud_unfolder<IUS, ISNE, IN, IW, IMV, IGL, IMI, ISAU, IGAU, ISABG, ISL, ISC, ISP, IGP,
             IGI, IACI, ISI, IRB, IGACR, IRU>::
materialize_child(pud_candidate_search_context& ctx,
                  const pud_rule_id* leaf,
                  size_t body_goal_idx,
                  uint32_t parent_lvc) {
    const pud_rule_id* id = make_inference_.make_inference(leaf, body_goal_idx, ctx.cursor);
    set_norm_env_.set_normalization_environment(get_interval_.get(id), parent_lvc);

    std::vector<const pud_rule_id*> ancestors;
    for (const pud_rule_id* node = get_parent_.get(ctx.cursor); node != nullptr;
         node = get_parent_.get(node))
        ancestors.push_back(node);

    std::vector<uint32_t> caller_reps;
    for (size_t idx = ancestors.size(); idx > 0; --idx) {
        const pud_rule_id* interned = make_inference_.make_inference(
            leaf, body_goal_idx, ancestors[idx - 1]);
        const std::vector<uint32_t>& delta = get_added_caller_reps_.get(interned);
        caller_reps.insert(caller_reps.end(), delta.begin(), delta.end());
    }
    const std::vector<uint32_t>& cursor_delta = get_added_caller_reps_.get(id);
    caller_reps.insert(caller_reps.end(), cursor_delta.begin(), cursor_delta.end());
    for (uint32_t rep = 0; rep < parent_lvc; ++rep) {
        bool already_listed = false;
        for (uint32_t existing : caller_reps) {
            if (existing != rep)
                continue;
            already_listed = true;
            break;
        }
        if (already_listed)
            continue;
        framed_expr reduced = whnf_.whnf(framed_expr{make_var_.make_var(rep), 0});
        const expr::var* reduced_var = std::get_if<expr::var>(&reduced.skeleton->content);
        const bool unbound_self = reduced_var != nullptr
            && reduced.frame_offset == 0
            && reduced_var->index == rep;
        if (unbound_self)
            continue;
        caller_reps.push_back(rep);
    }

    std::unordered_map<uint32_t, uint32_t> translation;
    std::vector<pud_added_unification> caller_snapshots;
    for (uint32_t rep : caller_reps) {
        framed_expr reduced = whnf_.whnf(framed_expr{make_var_.make_var(rep), 0});
        const expr* value = normalize_.normalize(reduced, translation);
        caller_snapshots.push_back(pud_added_unification{rep, value});
    }
    const std::vector<pud_added_unification>& leaf_unifs = get_added_unifications_.get(leaf);
    DEBUG_ASSERT(!leaf_unifs.empty());
    framed_expr reduced_head = whnf_.whnf(framed_expr{leaf_unifs[0].value, 0});
    const expr* spec_head = normalize_.normalize(reduced_head, translation);

    std::vector<const expr*> added_body_goals;
    for (const expr* goal : ctx.added_body_goals) {
        added_body_goals.push_back(normalize_.normalize(
            framed_expr{goal, parent_lvc}, translation));
    }
    const uint32_t child_lvc = parent_lvc
        + static_cast<uint32_t>(translation.size());

    const om_interval clean = allocate_child_interval_.allocate_child_of(
        get_interval_.get(leaf));
    for (const pud_added_unification& added : caller_snapshots) {
        record_binding_.record(
            clean, added.var_idx, framed_expr{added.value, 0});
    }
    std::vector<pud_added_unification> added_unifications;
    added_unifications.push_back(pud_added_unification{0, spec_head});
    added_unifications.insert(
        added_unifications.end(), caller_snapshots.begin(), caller_snapshots.end());
    store_added_unifications_.store(id, std::move(added_unifications));
    store_added_body_goals_.store(id, std::move(added_body_goals));
    store_lvc_.store(id, child_lvc);
    store_interval_.store(id, clean);
    return id;
}

template<typename IUS, typename ISNE, typename IN, typename IW, typename IMV,
         typename IGL, typename IMI, typename ISAU, typename IGAU, typename ISABG, typename ISL,
         typename ISC, typename ISP, typename IGP, typename IGI, typename IACI,
         typename ISI, typename IRB, typename IGACR, typename IRU>
coroutine<pud_forced_unfold, std::vector<const pud_rule_id*>>
pud_unfolder<IUS, ISNE, IN, IW, IMV, IGL, IMI, ISAU, IGAU, ISABG, ISL, ISC, ISP, IGP,
             IGI, IACI, ISI, IRB, IGACR, IRU>::
unfold(const pud_rule_id* leaf, size_t body_goal_idx) {
    const pud_unfold_site site = unfold_site_.unfold_site(leaf, body_goal_idx);
    DEBUG_ASSERT(!site.live.empty());
    const uint32_t parent_lvc = get_lvc_.get(leaf);

    std::vector<const pud_rule_id*> children;
    for (pud_candidate_search_context* ctx : site.live)
        children.push_back(materialize_child(*ctx, leaf, body_goal_idx, parent_lvc));

    store_children_.store(leaf, {children.begin(), children.end()});
    for (const pud_rule_id* child : children)
        store_parent_.store(child, leaf);
    const std::vector<pud_forced_unfold> yields =
        replace_unfolded_.replace_unfolded(leaf, body_goal_idx, children);
    for (const pud_forced_unfold& yield : yields)
        co_yield yield;
    co_return children;
}

#endif
