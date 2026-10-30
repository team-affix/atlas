#ifndef PUD_UNFOLD_ALLOWER_HPP
#define PUD_UNFOLD_ALLOWER_HPP

#include <cstddef>
#include "value_objects/pud_global_node_id.hpp"

template<typename ICountNodeDepth, typename IGetNodeGoals>
struct pud_unfold_allower {
    pud_unfold_allower(ICountNodeDepth& count_node_depth, IGetNodeGoals& get_node_goals, size_t max_node_depth);
    bool allow(pud_global_node_id node) const;
private:
    ICountNodeDepth& count_node_depth_;
    IGetNodeGoals& get_node_goals_;
    size_t max_node_depth_;
};

template<typename ICountNodeDepth, typename IGetNodeGoals>
pud_unfold_allower<ICountNodeDepth, IGetNodeGoals>::pud_unfold_allower(
    ICountNodeDepth& count_node_depth, IGetNodeGoals& get_node_goals, size_t max_node_depth)
    : count_node_depth_(count_node_depth)
    , get_node_goals_(get_node_goals)
    , max_node_depth_(max_node_depth) {}

template<typename ICountNodeDepth, typename IGetNodeGoals>
bool pud_unfold_allower<ICountNodeDepth, IGetNodeGoals>::allow(pud_global_node_id node) const {
    constexpr size_t k_max_open_goals = 64;
    if (count_node_depth_.count(node) >= max_node_depth_)
        return false;
    return get_node_goals_.get(node).goals.size() <= k_max_open_goals;
}

#endif
