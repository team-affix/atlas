#ifndef PUD_QUERY_HOLDERS_HPP
#define PUD_QUERY_HOLDERS_HPP

#include <set>
#include <unordered_map>
#include <utility>
#include <vector>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_mhq_head_id.hpp"
#include "value_objects/pud_node_id.hpp"

template<
    typename Descent,
    typename IStoreNodeDescent,
    typename IEraseNodeDescent,
    typename ICheckNodeDescent,
    typename IGetNodeDescent>
struct pud_query_holders {
    pud_query_holders(
        IStoreNodeDescent& store_node_descent,
        IEraseNodeDescent& erase_node_descent,
        ICheckNodeDescent& check_node_descent,
        IGetNodeDescent& get_node_descent);
    void store(pud_node_id leaf, Descent descent);
    void erase(pud_node_id leaf);
    std::vector<std::pair<pud_node_id, pud_body_goal_id>> get(pud_mhq_head_id head_id) const;
private:
    IStoreNodeDescent& store_node_descent_;
    IEraseNodeDescent& erase_node_descent_;
    ICheckNodeDescent& check_node_descent_;
    IGetNodeDescent& get_node_descent_;
    std::unordered_map<pud_mhq_head_id, std::set<std::pair<pud_node_id, pud_body_goal_id>>> holders_;
};

#define PUD_QUERY_HOLDERS_TEMPLATE                                                         \
    template<typename Descent, typename IStoreNodeDescent, typename IEraseNodeDescent,    \
             typename ICheckNodeDescent, typename IGetNodeDescent>
#define PUD_QUERY_HOLDERS                                                                  \
    pud_query_holders<Descent, IStoreNodeDescent, IEraseNodeDescent, ICheckNodeDescent,    \
        IGetNodeDescent>

PUD_QUERY_HOLDERS_TEMPLATE
PUD_QUERY_HOLDERS::pud_query_holders(
    IStoreNodeDescent& store_node_descent,
    IEraseNodeDescent& erase_node_descent,
    ICheckNodeDescent& check_node_descent,
    IGetNodeDescent& get_node_descent)
    : store_node_descent_(store_node_descent)
    , erase_node_descent_(erase_node_descent)
    , check_node_descent_(check_node_descent)
    , get_node_descent_(get_node_descent)
    , holders_() {}

PUD_QUERY_HOLDERS_TEMPLATE
void PUD_QUERY_HOLDERS::store(pud_node_id leaf, Descent descent) {
    for (const auto& [goal, head_id] : descent.body_goal_pq().get_query_heads())
        holders_[head_id].emplace(leaf, goal);
    store_node_descent_.store(leaf, std::move(descent));
}

PUD_QUERY_HOLDERS_TEMPLATE
void PUD_QUERY_HOLDERS::erase(pud_node_id leaf) {
    if (!check_node_descent_.check_descent(leaf))
        return;
    for (const auto& [goal, head_id] : get_node_descent_.get(leaf).body_goal_pq().get_query_heads()) {
        auto holders_it = holders_.find(head_id);
        holders_it->second.erase({leaf, goal});
        if (holders_it->second.empty())
            holders_.erase(holders_it);
    }
    erase_node_descent_.erase(leaf);
}

PUD_QUERY_HOLDERS_TEMPLATE
std::vector<std::pair<pud_node_id, pud_body_goal_id>> PUD_QUERY_HOLDERS::get(pud_mhq_head_id head_id) const {
    const auto holders_it = holders_.find(head_id);
    if (holders_it == holders_.end())
        return {};
    return {holders_it->second.begin(), holders_it->second.end()};
}

#undef PUD_QUERY_HOLDERS
#undef PUD_QUERY_HOLDERS_TEMPLATE

#endif
