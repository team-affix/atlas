#ifndef PUD_SOLVER_HPP
#define PUD_SOLVER_HPP

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"

// Grows a query's tree breadth-first: the oldest live leaf is expanded at its first open goal
// until some live leaf has no open goals left (a solution), the tree is refuted, or the budget
// runs out.
template<
    typename IFindLiveLeaves,
    typename IGetNodeDescent,
    typename IExpandLeaf,
    typename IMakeRootDescent,
    typename IStoreNodeDescent,
    typename IRefuteNode>
struct pud_solver {
    pud_solver(
        IFindLiveLeaves& find_live_leaves,
        IGetNodeDescent& get_node_descent,
        IExpandLeaf& expand_leaf,
        IMakeRootDescent& make_root_descent,
        IStoreNodeDescent& store_node_descent,
        IRefuteNode& refute_node);
    void start(pud_node_id query_root);
    std::optional<pud_node_id> solve(pud_node_id query_root, size_t max_expansions);
private:
    IFindLiveLeaves& find_live_leaves_;
    IGetNodeDescent& get_node_descent_;
    IExpandLeaf& expand_leaf_;
    IMakeRootDescent& make_root_descent_;
    IStoreNodeDescent& store_node_descent_;
    IRefuteNode& refute_node_;
};

#define PUD_SOLVER_TEMPLATE                                                                \
    template<typename IFindLiveLeaves, typename IGetNodeDescent, typename IExpandLeaf,     \
             typename IMakeRootDescent, typename IStoreNodeDescent, typename IRefuteNode>
#define PUD_SOLVER                                                                         \
    pud_solver<IFindLiveLeaves, IGetNodeDescent, IExpandLeaf, IMakeRootDescent,            \
        IStoreNodeDescent, IRefuteNode>

PUD_SOLVER_TEMPLATE
PUD_SOLVER::pud_solver(
    IFindLiveLeaves& find_live_leaves,
    IGetNodeDescent& get_node_descent,
    IExpandLeaf& expand_leaf,
    IMakeRootDescent& make_root_descent,
    IStoreNodeDescent& store_node_descent,
    IRefuteNode& refute_node)
    : find_live_leaves_(find_live_leaves)
    , get_node_descent_(get_node_descent)
    , expand_leaf_(expand_leaf)
    , make_root_descent_(make_root_descent)
    , store_node_descent_(store_node_descent)
    , refute_node_(refute_node) {}

PUD_SOLVER_TEMPLATE
void PUD_SOLVER::start(pud_node_id query_root) {
    auto root_descent = make_root_descent_.make(query_root);
    const bool goal_refuted = !root_descent.body_goal_pq().refuted_goals().empty();
    store_node_descent_.store(query_root, std::move(root_descent));
    if (!goal_refuted)
        return;
    refute_node_.refute(query_root);
}

PUD_SOLVER_TEMPLATE
std::optional<pud_node_id> PUD_SOLVER::solve(pud_node_id query_root, size_t max_expansions) {
    for (size_t expansion = 0;; ++expansion) {
        const std::vector<pud_node_id> live_leaves = find_live_leaves_.find_live_leaves(query_root);
        if (live_leaves.empty())
            return std::nullopt;
        const pud_node_id oldest_leaf = *std::min_element(live_leaves.begin(), live_leaves.end());
        const auto& open_goals = get_node_descent_.get(oldest_leaf).body_goal_expr().body_goal_exprs();
        if (open_goals.empty())
            return oldest_leaf;
        if (expansion == max_expansions)
            return std::nullopt;
        body_goal_id first_goal = open_goals.begin()->first;
        for (const auto& [goal, goal_expr] : open_goals)
            first_goal = std::min(first_goal, goal);
        expand_leaf_.expand(oldest_leaf, first_goal);
    }
}

#undef PUD_SOLVER
#undef PUD_SOLVER_TEMPLATE

#endif
