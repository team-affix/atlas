#ifndef PUD_LODESTAR_BLOCKED_REFUTER_HPP
#define PUD_LODESTAR_BLOCKED_REFUTER_HPP

#include <vector>
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_lodestar_query.hpp"

// Refutes the first conjecture leaf that has a goal without candidates, and takes what that refutes
// off the frontier.
template<typename IListQueries, typename IRefuteLeaf, typename IEraseRefuted>
struct pud_lodestar_blocked_refuter {
    pud_lodestar_blocked_refuter(IListQueries& list_queries, IRefuteLeaf& refute_leaf, IEraseRefuted& erase_refuted);
    bool refute_blocked();
private:
    IListQueries& list_queries_;
    IRefuteLeaf& refute_leaf_;
    IEraseRefuted& erase_refuted_;
};

template<typename IListQueries, typename IRefuteLeaf, typename IEraseRefuted>
pud_lodestar_blocked_refuter<IListQueries, IRefuteLeaf, IEraseRefuted>::pud_lodestar_blocked_refuter(
    IListQueries& list_queries, IRefuteLeaf& refute_leaf, IEraseRefuted& erase_refuted)
    : list_queries_(list_queries)
    , refute_leaf_(refute_leaf)
    , erase_refuted_(erase_refuted) {}

template<typename IListQueries, typename IRefuteLeaf, typename IEraseRefuted>
bool pud_lodestar_blocked_refuter<IListQueries, IRefuteLeaf, IEraseRefuted>::refute_blocked() {
    for (const pud_lodestar_query& query : list_queries_.list()) {
        if (query.has_candidates)
            continue;
        const std::vector<pud_global_node_id> refuted = refute_leaf_.refute(query.node);
        if (refuted.empty())
            erase_refuted_.erase_refuted({query.node});
        else
            erase_refuted_.erase_refuted(refuted);
        return true;
    }
    return false;
}

#endif
