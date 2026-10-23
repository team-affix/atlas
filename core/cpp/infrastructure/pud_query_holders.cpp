#include "infrastructure/pud_query_holders.hpp"
#include "debug_assert.hpp"

pud_query_holders::pud_query_holders()
    : holdings_() {}

void pud_query_holders::hold(pud_mhq_head_id head_id, pud_node_id leaf, pud_body_goal_id goal) {
    holding& held = holdings_.try_emplace(head_id, holding{.goal = goal, .leaves = {}}).first->second;
    DEBUG_ASSERT(held.goal == goal);
    [[maybe_unused]] const bool inserted = held.leaves.insert(leaf).second;
    DEBUG_ASSERT(inserted);
}

// Returns whether the leaf was the query's last holder; the query is then forgotten here.
bool pud_query_holders::release(pud_mhq_head_id head_id, pud_node_id leaf) {
    holding& held = holdings_.at(head_id);
    [[maybe_unused]] const size_t erased = held.leaves.erase(leaf);
    DEBUG_ASSERT(erased == 1);
    if (!held.leaves.empty())
        return false;
    holdings_.erase(head_id);
    return true;
}

std::optional<pud_unfold> pud_query_holders::find_holder(pud_mhq_head_id head_id) const {
    const auto held = holdings_.find(head_id);
    if (held == holdings_.end())
        return std::nullopt;
    return pud_unfold{.leaf = *held->second.leaves.begin(), .goal = held->second.goal};
}

std::vector<pud_node_id> pud_query_holders::get_holders(pud_mhq_head_id head_id) const {
    const std::set<pud_node_id>& leaves = holdings_.at(head_id).leaves;
    return std::vector<pud_node_id>(leaves.begin(), leaves.end());
}
