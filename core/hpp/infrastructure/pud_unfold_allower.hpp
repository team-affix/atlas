#ifndef PUD_UNFOLD_ALLOWER_HPP
#define PUD_UNFOLD_ALLOWER_HPP

#include <cstddef>
#include "value_objects/pud_global_node_id.hpp"

template<typename IGetNodeDepth, typename IGetNodeGoals>
struct pud_unfold_allower {
    pud_unfold_allower(
        IGetNodeDepth& get_node_depth,
        IGetNodeGoals& get_node_goals,
        size_t max_node_depth,
        size_t max_open_goals);
    bool allow(pud_global_node_id node) const;
private:
    IGetNodeDepth& get_node_depth_;
    IGetNodeGoals& get_node_goals_;
    size_t max_node_depth_;
    size_t max_open_goals_;
};

template<typename IGetNodeDepth, typename IGetNodeGoals>
pud_unfold_allower<IGetNodeDepth, IGetNodeGoals>::pud_unfold_allower(
    IGetNodeDepth& get_node_depth,
    IGetNodeGoals& get_node_goals,
    size_t max_node_depth,
    size_t max_open_goals)
    : get_node_depth_(get_node_depth)
    , get_node_goals_(get_node_goals)
    , max_node_depth_(max_node_depth)
    , max_open_goals_(max_open_goals) {}

template<typename IGetNodeDepth, typename IGetNodeGoals>
bool pud_unfold_allower<IGetNodeDepth, IGetNodeGoals>::allow(pud_global_node_id node) const {
    if (get_node_depth_.get(node) >= max_node_depth_)
        return false;
    return get_node_goals_.get(node).goals.size() <= max_open_goals_;
}

#endif
