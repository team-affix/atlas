#ifndef PUD_BLOCKED_LEAF_REFUTER_HPP
#define PUD_BLOCKED_LEAF_REFUTER_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_leaf_goal_query.hpp"
#include "debug_assert.hpp"

// Refutes the first listed leaf that has a goal without candidates, and takes what that refutes
// off the frontier.
template<typename IRefuteLeaf, typename IEraseRefuted>
struct pud_blocked_leaf_refuter {
    pud_blocked_leaf_refuter(IRefuteLeaf& refute_leaf, IEraseRefuted& erase_refuted);
    bool refute_blocked(const std::vector<pud_leaf_goal_query>& queries);
private:
    IRefuteLeaf& refute_leaf_;
    IEraseRefuted& erase_refuted_;
};

template<typename IRefuteLeaf, typename IEraseRefuted>
pud_blocked_leaf_refuter<IRefuteLeaf, IEraseRefuted>::pud_blocked_leaf_refuter(
    IRefuteLeaf& refute_leaf, IEraseRefuted& erase_refuted)
    : refute_leaf_(refute_leaf)
    , erase_refuted_(erase_refuted) {}

template<typename IRefuteLeaf, typename IEraseRefuted>
bool pud_blocked_leaf_refuter<IRefuteLeaf, IEraseRefuted>::refute_blocked(
    const std::vector<pud_leaf_goal_query>& queries) {
    for (const pud_leaf_goal_query& query : queries) {
        if (query.has_candidates)
            continue;
        const std::vector<pud_global_node_id> refuted = refute_leaf_.refute(query.node);
        DEBUG_ASSERT(!refuted.empty());
        erase_refuted_.erase_refuted(refuted);
        return true;
    }
    return false;
}

#endif
