#ifndef PUD_WITNESS_SEARCH_HPP
#define PUD_WITNESS_SEARCH_HPP

#include <cstdint>
#include <optional>
#include <set>
#include <vector>
#include "infrastructure/hierarchical_bind_map.hpp"
#include "infrastructure/unifier.hpp"
#include "value_objects/framed_expr.hpp"
#include "value_objects/om_interval.hpp"
#include "value_objects/pud_added_unification.hpp"
#include "value_objects/pud_rule_id.hpp"
#include "value_objects/pud_witness_search_context.hpp"
#include "debug_assert.hpp"

template<typename IGetChildren,
         typename IGetParent,
         typename IMakeInference,
         typename IContainsInterval,
         typename IGetInterval,
         typename IStoreInterval,
         typename IAllocateChildInterval,
         typename IGetAddedUnifications,
         typename IRecordBinding,
         typename IQueryBinding,
         typename IGlobalize,
         typename IMakeVar,
         typename IStoreAddedCallerReps>
struct pud_witness_search {
    pud_witness_search(IGetChildren& get_children,
                       IGetParent& get_parent,
                       IMakeInference& make_inference,
                       IContainsInterval& contains_interval,
                       IGetInterval& get_interval,
                       IStoreInterval& store_interval,
                       IAllocateChildInterval& allocate_child_interval,
                       IGetAddedUnifications& get_added_unifications,
                       IRecordBinding& record_binding,
                       IQueryBinding& query_binding,
                       IGlobalize& globalize,
                       IMakeVar& make_var,
                       IStoreAddedCallerReps& store_added_caller_reps);
    void resume(pud_witness_search_context& context);
private:
    using bind_map_t = hierarchical_bind_map<IGlobalize, IRecordBinding, IQueryBinding>;
    using unifier_t = unifier<IGlobalize, bind_map_t>;

    bool try_enter(pud_witness_search_context& context, const pud_rule_id* node);
    bool try_subtree(pud_witness_search_context& context, const pud_rule_id* node);

    IGetChildren& get_children_;
    IGetParent& get_parent_;
    IMakeInference& make_inference_;
    IContainsInterval& contains_interval_;
    IGetInterval& get_interval_;
    IStoreInterval& store_interval_;
    IAllocateChildInterval& allocate_child_interval_;
    IGetAddedUnifications& get_added_unifications_;
    IRecordBinding& record_binding_;
    IQueryBinding& query_binding_;
    IGlobalize& globalize_;
    IMakeVar& make_var_;
    IStoreAddedCallerReps& store_added_caller_reps_;
};

template<typename IGC, typename IGP, typename IMI, typename ICI, typename IGI,
         typename ISI, typename IACI, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename ISACR>
pud_witness_search<IGC, IGP, IMI, ICI, IGI, ISI, IACI, IGAU, IRB, IQB, IG, IMV, ISACR>::
pud_witness_search(IGC& get_children,
                   IGP& get_parent,
                   IMI& make_inference,
                   ICI& contains_interval,
                   IGI& get_interval,
                   ISI& store_interval,
                   IACI& allocate_child_interval,
                   IGAU& get_added_unifications,
                   IRB& record_binding,
                   IQB& query_binding,
                   IG& globalize,
                   IMV& make_var,
                   ISACR& store_added_caller_reps)
    : get_children_(get_children)
    , get_parent_(get_parent)
    , make_inference_(make_inference)
    , contains_interval_(contains_interval)
    , get_interval_(get_interval)
    , store_interval_(store_interval)
    , allocate_child_interval_(allocate_child_interval)
    , get_added_unifications_(get_added_unifications)
    , record_binding_(record_binding)
    , query_binding_(query_binding)
    , globalize_(globalize)
    , make_var_(make_var)
    , store_added_caller_reps_(store_added_caller_reps) {}

template<typename IGC, typename IGP, typename IMI, typename ICI, typename IGI,
         typename ISI, typename IACI, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename ISACR>
bool pud_witness_search<IGC, IGP, IMI, ICI, IGI, ISI, IACI, IGAU, IRB, IQB, IG, IMV, ISACR>::
try_enter(pud_witness_search_context& context, const pud_rule_id* node) {
    const pud_rule_id* key = make_inference_.make_inference(
        context.query_leaf, context.body_goal_idx, node);
    if (contains_interval_.contains(key))
        return true;
    const pud_rule_id* forest_parent = get_parent_.get(node);
    const pud_rule_id* parent_key = make_inference_.make_inference(
        context.query_leaf, context.body_goal_idx, forest_parent);
    const om_interval interval = allocate_child_interval_.allocate_child_of(
        get_interval_.get(parent_key));
    bind_map_t bm(globalize_, record_binding_, query_binding_, interval);
    unifier_t task_owner(globalize_, &bm);
    std::vector<uint32_t> added_caller_reps;
    for (const pud_added_unification& added : get_added_unifications_.get(node)) {
        auto task = task_owner.unify(
            framed_expr{make_var_.make_var(added.var_idx), context.frame_offset},
            framed_expr{added.value, context.frame_offset});
        while (!task.done()) {
            task.resume();
            if (!task.has_yield())
                continue;
            const uint32_t rep = task.consume_yield();
            if (rep == 0)
                continue;
            if (rep >= context.frame_offset)
                continue;
            added_caller_reps.push_back(rep);
        }
        if (task.result())
            continue;
        return false;
    }
    store_interval_.store(key, interval);
    store_added_caller_reps_.store(key, std::move(added_caller_reps));
    return true;
}

template<typename IGC, typename IGP, typename IMI, typename ICI, typename IGI,
         typename ISI, typename IACI, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename ISACR>
bool pud_witness_search<IGC, IGP, IMI, ICI, IGI, ISI, IACI, IGAU, IRB, IQB, IG, IMV, ISACR>::
try_subtree(pud_witness_search_context& context, const pud_rule_id* node) {
    if (!try_enter(context, node))
        return false;
    const std::optional<std::set<const pud_rule_id*>> children = get_children_.get(node);
    if (!children.has_value()) {
        context.current = node;
        return true;
    }
    for (const pud_rule_id* child : *children) {
        if (try_subtree(context, child))
            return true;
    }
    return false;
}

template<typename IGC, typename IGP, typename IMI, typename ICI, typename IGI,
         typename ISI, typename IACI, typename IGAU, typename IRB, typename IQB,
         typename IG, typename IMV, typename ISACR>
void pud_witness_search<IGC, IGP, IMI, ICI, IGI, ISI, IACI, IGAU, IRB, IQB, IG, IMV, ISACR>::
resume(pud_witness_search_context& context) {
    if (context.current == nullptr)
        return;
    std::vector<const pud_rule_id*> chain;
    for (const pud_rule_id* node = context.current; node != nullptr;
            node = get_parent_.get(node))
        chain.push_back(node);
    const pud_rule_id* from = context.current;
    bool path_ok = true;
    for (size_t idx = chain.size(); idx > 0; --idx) {
        if (try_enter(context, chain[idx - 1]))
            continue;
        from = chain[idx - 1];
        path_ok = false;
        break;
    }
    if (!path_ok) {
        const pud_rule_id* ancestor = get_parent_.get(context.search_root);
        while (ancestor != nullptr) {
            if (ancestor == from) {
                context.current = nullptr;
                return;
            }
            ancestor = get_parent_.get(ancestor);
        }
    } else if (!get_children_.get(context.current).has_value()) {
        return;
    } else if (try_subtree(context, from)) {
        return;
    }
    const pud_rule_id* walk = from;
    while (walk != context.search_root) {
        const pud_rule_id* parent = get_parent_.get(walk);
        DEBUG_ASSERT(parent != nullptr);
        const std::optional<std::set<const pud_rule_id*>> siblings =
            get_children_.get(parent);
        DEBUG_ASSERT(siblings.has_value());
        bool past_walk = false;
        for (const pud_rule_id* sibling : *siblings) {
            if (!past_walk) {
                if (sibling == walk)
                    past_walk = true;
                continue;
            }
            if (try_subtree(context, sibling))
                return;
        }
        walk = parent;
    }
    context.current = nullptr;
}

#endif
