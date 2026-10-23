#ifndef PUD_LEAF_RELEASER_HPP
#define PUD_LEAF_RELEASER_HPP

#include <immer/map.hpp>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Takes a leaf off the frontier: it lets go of every query it holds, and a query no leaf holds any more
// is removed.
template<typename IGetLeafState, typename IEraseLeafState, typename IReleaseQuery, typename IRemoveQuery>
struct pud_leaf_releaser {
    pud_leaf_releaser(
        IGetLeafState& get_leaf_state,
        IEraseLeafState& erase_leaf_state,
        IReleaseQuery& release_query,
        IRemoveQuery& remove_query);
    void release(pud_node_id leaf);
private:
    IGetLeafState& get_leaf_state_;
    IEraseLeafState& erase_leaf_state_;
    IReleaseQuery& release_query_;
    IRemoveQuery& remove_query_;
};

template<typename IGLS, typename IELS, typename IRelQ, typename IRemQ>
pud_leaf_releaser<IGLS, IELS, IRelQ, IRemQ>::pud_leaf_releaser(
    IGLS& get_leaf_state,
    IELS& erase_leaf_state,
    IRelQ& release_query,
    IRemQ& remove_query)
    : get_leaf_state_(get_leaf_state)
    , erase_leaf_state_(erase_leaf_state)
    , release_query_(release_query)
    , remove_query_(remove_query) {}

template<typename IGLS, typename IELS, typename IRelQ, typename IRemQ>
void pud_leaf_releaser<IGLS, IELS, IRelQ, IRemQ>::release(pud_node_id leaf) {
    for (const auto& [goal, head_id] : get_leaf_state_.get(leaf).queries) {
        if (!release_query_.release(head_id, leaf))
            continue;
        remove_query_.remove(head_id);
    }
    erase_leaf_state_.erase(leaf);
}

#endif
