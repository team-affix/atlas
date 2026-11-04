#ifndef PUD_UNFOLDER_HPP
#define PUD_UNFOLDER_HPP

#include <utility>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_frontier_change.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_inference.hpp"
#include "debug_assert.hpp"

template<
    typename IResolve, typename IShuffleChildren, typename IStoreInference, typename IMoveActivity,
    typename IIsNodeRefuted>
struct pud_unfolder {
    pud_unfolder(
        IResolve& resolve,
        IShuffleChildren& shuffle_children,
        IStoreInference& store_inference,
        IMoveActivity& move_activity,
        IIsNodeRefuted& is_node_refuted);
    pud_frontier_change unfold(pud_global_node_id leaf, pud_body_goal_id goal);
private:
    IResolve& resolve_;
    IShuffleChildren& shuffle_children_;
    IStoreInference& store_inference_;
    IMoveActivity& move_activity_;
    IIsNodeRefuted& is_node_refuted_;
};

#define PUD_UNFOLDER_TEMPLATE                                                                             \
    template<typename IResolve, typename IShuffleChildren, typename IStoreInference, typename IMoveActivity, \
             typename IIsNodeRefuted>
#define PUD_UNFOLDER pud_unfolder<IResolve, IShuffleChildren, IStoreInference, IMoveActivity, IIsNodeRefuted>

PUD_UNFOLDER_TEMPLATE
PUD_UNFOLDER::pud_unfolder(
    IResolve& resolve,
    IShuffleChildren& shuffle_children,
    IStoreInference& store_inference,
    IMoveActivity& move_activity,
    IIsNodeRefuted& is_node_refuted)
    : resolve_(resolve)
    , shuffle_children_(shuffle_children)
    , store_inference_(store_inference)
    , move_activity_(move_activity)
    , is_node_refuted_(is_node_refuted) {}

PUD_UNFOLDER_TEMPLATE
pud_frontier_change PUD_UNFOLDER::unfold(pud_global_node_id leaf, pud_body_goal_id goal) {
    std::vector<pud_global_node_id> children;
    for (pud_inference& inference : shuffle_children_.shuffle(leaf, resolve_.resolve(leaf, goal)))
        children.push_back(store_inference_.store_inference(leaf.module, std::move(inference)));
    DEBUG_ASSERT(!children.empty());
    pud_frontier_change change{.offspring = {}, .refuted = move_activity_.move(leaf, children)};
    for (const pud_global_node_id child : children) {
        if (is_node_refuted_.is_refuted(child))
            continue;
        change.offspring.push_back(child);
    }
    return change;
}

#undef PUD_UNFOLDER
#undef PUD_UNFOLDER_TEMPLATE

#endif
