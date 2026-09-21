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
         typename IUnifyCallee,
         typename INormalize,
         typename IMakeVar,
         typename IGetLvc,
         typename IMakeInference,
         typename IStoreAddedUnifications,
         typename IStoreAddedBodyGoals,
         typename IStoreLvc,
         typename IStoreChildren,
         typename IStoreParent,
         typename IGetBaseInterval,
         typename IAllocateChildInterval,
         typename IStoreBaseInterval,
         typename IReplaceUnfolded>
struct pud_unfolder {
    pud_unfolder(IUnfoldSite& unfold_site,
                 IUnifyCallee& unify_callee,
                 INormalize& normalize,
                 IMakeVar& make_var,
                 IGetLvc& get_lvc,
                 IMakeInference& make_inference,
                 IStoreAddedUnifications& store_added_unifications,
                 IStoreAddedBodyGoals& store_added_body_goals,
                 IStoreLvc& store_lvc,
                 IStoreChildren& store_children,
                 IStoreParent& store_parent,
                 IGetBaseInterval& get_base_interval,
                 IAllocateChildInterval& allocate_child_interval,
                 IStoreBaseInterval& store_base_interval,
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
    IUnifyCallee& unify_callee_;
    INormalize& normalize_;
    IMakeVar& make_var_;
    IGetLvc& get_lvc_;
    IMakeInference& make_inference_;
    IStoreAddedUnifications& store_added_unifications_;
    IStoreAddedBodyGoals& store_added_body_goals_;
    IStoreLvc& store_lvc_;
    IStoreChildren& store_children_;
    IStoreParent& store_parent_;
    IGetBaseInterval& get_base_interval_;
    IAllocateChildInterval& allocate_child_interval_;
    IStoreBaseInterval& store_base_interval_;
    IReplaceUnfolded& replace_unfolded_;
};

template<typename IUS, typename IUC, typename IN, typename IMV, typename IGL,
         typename IMI, typename ISAU, typename ISABG, typename ISL,
         typename ISC, typename ISP, typename IGBI, typename IACI, typename ISBI,
         typename IRU>
pud_unfolder<IUS, IUC, IN, IMV, IGL, IMI, ISAU, ISABG, ISL, ISC, ISP, IGBI, IACI, ISBI, IRU>::
pud_unfolder(IUS& unfold_site,
             IUC& unify_callee,
             IN& normalize,
             IMV& make_var,
             IGL& get_lvc,
             IMI& make_inference,
             ISAU& store_added_unifications,
             ISABG& store_added_body_goals,
             ISL& store_lvc,
             ISC& store_children,
             ISP& store_parent,
             IGBI& get_base_interval,
             IACI& allocate_child_interval,
             ISBI& store_base_interval,
             IRU& replace_unfolded)
    : unfold_site_(unfold_site)
    , unify_callee_(unify_callee)
    , normalize_(normalize)
    , make_var_(make_var)
    , get_lvc_(get_lvc)
    , make_inference_(make_inference)
    , store_added_unifications_(store_added_unifications)
    , store_added_body_goals_(store_added_body_goals)
    , store_lvc_(store_lvc)
    , store_children_(store_children)
    , store_parent_(store_parent)
    , get_base_interval_(get_base_interval)
    , allocate_child_interval_(allocate_child_interval)
    , store_base_interval_(store_base_interval)
    , replace_unfolded_(replace_unfolded) {}

template<typename IUS, typename IUC, typename IN, typename IMV, typename IGL,
         typename IMI, typename ISAU, typename ISABG, typename ISL,
         typename ISC, typename ISP, typename IGBI, typename IACI, typename ISBI,
         typename IRU>
const pud_rule_id*
pud_unfolder<IUS, IUC, IN, IMV, IGL, IMI, ISAU, ISABG, ISL, ISC, ISP, IGBI, IACI, ISBI, IRU>::
materialize_child(pud_candidate_search_context& ctx,
                  const pud_rule_id* leaf,
                  size_t body_goal_idx,
                  uint32_t parent_lvc) {
    std::vector<uint32_t> touched_reps;
    om_interval env{ctx.interval};
    const bool unified = unify_callee_.unify_callee(
        ctx, ctx.cursor, touched_reps, env);
    DEBUG_ASSERT(unified);

    std::unordered_map<uint32_t, uint32_t> translation;
    std::vector<pud_added_unification> added_unifications;
    for (uint32_t rep : touched_reps) {
        const expr* value = normalize_.normalize(
            env,
            framed_expr{make_var_.make_var(rep), 0},
            parent_lvc,
            translation);
        added_unifications.push_back(pud_added_unification{rep, value});
    }

    std::vector<const expr*> added_body_goals;
    for (const expr* goal : ctx.added_body_goals) {
        added_body_goals.push_back(normalize_.normalize(
            env, framed_expr{goal, 0}, parent_lvc, translation));
    }
    const uint32_t child_lvc = parent_lvc
        + static_cast<uint32_t>(translation.size());

    const pud_rule_id* id = make_inference_.make_inference(leaf, body_goal_idx, ctx.cursor);
    store_added_unifications_.store(id, std::move(added_unifications));
    store_added_body_goals_.store(id, std::move(added_body_goals));
    store_lvc_.store(id, child_lvc);
    return id;
}

template<typename IUS, typename IUC, typename IN, typename IMV, typename IGL,
         typename IMI, typename ISAU, typename ISABG, typename ISL,
         typename ISC, typename ISP, typename IGBI, typename IACI, typename ISBI,
         typename IRU>
coroutine<pud_forced_unfold, std::vector<const pud_rule_id*>>
pud_unfolder<IUS, IUC, IN, IMV, IGL, IMI, ISAU, ISABG, ISL, ISC, ISP, IGBI, IACI, ISBI, IRU>::
unfold(const pud_rule_id* leaf, size_t body_goal_idx) {
    const pud_unfold_site site = unfold_site_.unfold_site(leaf, body_goal_idx);
    DEBUG_ASSERT(!site.live.empty());
    const uint32_t parent_lvc = get_lvc_.get(leaf);

    std::vector<const pud_rule_id*> children;
    for (pud_candidate_search_context* ctx : site.live)
        children.push_back(materialize_child(*ctx, leaf, body_goal_idx, parent_lvc));

    store_children_.store(leaf, {children.begin(), children.end()});
    for (const pud_rule_id* child : children) {
        store_parent_.store(child, leaf);
        store_base_interval_.store(
            child,
            allocate_child_interval_.allocate_child_of(get_base_interval_.get(leaf)));
    }
    const std::vector<pud_forced_unfold> yields =
        replace_unfolded_.replace_unfolded(leaf, body_goal_idx, children);
    for (const pud_forced_unfold& yield : yields)
        co_yield yield;
    co_return children;
}

#endif
