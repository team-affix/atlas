#ifndef PUD_SOLVER_HPP
#define PUD_SOLVER_HPP

#include <algorithm>
#include <optional>
#include <unordered_set>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/body_goal_id.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_solver_step.hpp"

// Grows a query's tree breadth-first, one step per resume: the oldest live leaf not yet yielded
// is either yielded as a solution (no open goals) or unfolded at its first open goal. Yielded
// solutions stay live, so the tree keeps them as witnesses and candidates. The search ends once
// every live leaf has been yielded.
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
    coroutine<pud_solver_step, void> solve(pud_node_id query_root);
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
coroutine<pud_solver_step, void> PUD_SOLVER::solve(pud_node_id query_root) {
    std::unordered_set<pud_node_id> yielded_solutions;
    while (true) {
        std::optional<pud_node_id> oldest_leaf;
        for (pud_node_id leaf : find_live_leaves_.find_live_leaves(query_root)) {
            if (yielded_solutions.contains(leaf))
                continue;
            if (oldest_leaf.has_value() && oldest_leaf.value() < leaf)
                continue;
            oldest_leaf = leaf;
        }
        if (!oldest_leaf.has_value())
            co_return;
        const pud_node_id leaf = oldest_leaf.value();
        const auto& open_goals = get_node_descent_.get(leaf).body_goal_expr().body_goal_exprs();
        if (open_goals.empty()) {
            yielded_solutions.insert(leaf);
            co_yield pud_solution{leaf};
            continue;
        }
        body_goal_id first_goal = open_goals.begin()->first;
        for (const auto& [goal, goal_expr] : open_goals)
            first_goal = std::min(first_goal, goal);
        expand_leaf_.expand(leaf, first_goal);
        co_yield pud_unfolded{leaf, first_goal};
    }
}

#undef PUD_SOLVER
#undef PUD_SOLVER_TEMPLATE

#endif
