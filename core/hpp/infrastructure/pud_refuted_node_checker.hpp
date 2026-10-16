#ifndef PUD_REFUTED_NODE_CHECKER_HPP
#define PUD_REFUTED_NODE_CHECKER_HPP

#include "value_objects/pud_node_id.hpp"

template<typename IGetLiveChildCount>
struct pud_refuted_node_checker {
    pud_refuted_node_checker(IGetLiveChildCount& get_live_child_count);
    bool check_refuted(pud_node_id node) const;
private:
    IGetLiveChildCount& get_live_child_count_;
};

template<typename IGetLiveChildCount>
pud_refuted_node_checker<IGetLiveChildCount>::pud_refuted_node_checker(IGetLiveChildCount& get_live_child_count)
    : get_live_child_count_(get_live_child_count) {}

template<typename IGetLiveChildCount>
bool pud_refuted_node_checker<IGetLiveChildCount>::check_refuted(pud_node_id node) const {
    return get_live_child_count_.get(node) == 0;
}

#endif
