#ifndef PUD_SOLVER_HPP
#define PUD_SOLVER_HPP

#include <optional>
#include <unordered_set>
#include <vector>
#include "infrastructure/coroutine.hpp"
#include "value_objects/pud_decision.hpp"
#include "value_objects/pud_node_id.hpp"
#include "value_objects/pud_solver_step.hpp"

// Grows a query's tree one step per resume: a live leaf with no open goals that was not yielded
// yet is yielded as a solution; otherwise the injected decision generator chooses which leaf to
// unfold over which goal. Yielded solutions stay live, so the tree keeps them as witnesses and
// candidates. The search ends once every live leaf has been yielded.
template<
    typename IFindLiveLeaves,
    typename IGetNodeDescent,
    typename IGenerateDecision,
    typename IExpandLeaf,
    typename IMakeRootDescent,
    typename IStoreNodeDescent,
    typename IRefuteNode>
struct pud_solver {
    pud_solver(
        IFindLiveLeaves& find_live_leaves,
        IGetNodeDescent& get_node_descent,
        IGenerateDecision& generate_decision,
        IExpandLeaf& expand_leaf,
        IMakeRootDescent& make_root_descent,
        IStoreNodeDescent& store_node_descent,
        IRefuteNode& refute_node);
    void start(pud_node_id query_root);
    coroutine<pud_solver_step, void> solve(pud_node_id query_root);
private:
    IFindLiveLeaves& find_live_leaves_;
    IGetNodeDescent& get_node_descent_;
    IGenerateDecision& generate_decision_;
    IExpandLeaf& expand_leaf_;
    IMakeRootDescent& make_root_descent_;
    IStoreNodeDescent& store_node_descent_;
    IRefuteNode& refute_node_;
};

#define PUD_SOLVER_TEMPLATE                                                                \
    template<typename IFindLiveLeaves, typename IGetNodeDescent, typename IGenerateDecision, \
             typename IExpandLeaf, typename IMakeRootDescent, typename IStoreNodeDescent,   \
             typename IRefuteNode>
#define PUD_SOLVER                                                                         \
    pud_solver<IFindLiveLeaves, IGetNodeDescent, IGenerateDecision, IExpandLeaf,           \
        IMakeRootDescent, IStoreNodeDescent, IRefuteNode>

PUD_SOLVER_TEMPLATE
PUD_SOLVER::pud_solver(
    IFindLiveLeaves& find_live_leaves,
    IGetNodeDescent& get_node_descent,
    IGenerateDecision& generate_decision,
    IExpandLeaf& expand_leaf,
    IMakeRootDescent& make_root_descent,
    IStoreNodeDescent& store_node_descent,
    IRefuteNode& refute_node)
    : find_live_leaves_(find_live_leaves)
    , get_node_descent_(get_node_descent)
    , generate_decision_(generate_decision)
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
        std::optional<pud_node_id> solution;
        std::vector<pud_node_id> open_leaves;
        for (pud_node_id leaf : find_live_leaves_.find_live_leaves(query_root)) {
            if (yielded_solutions.contains(leaf))
                continue;
            if (!get_node_descent_.get(leaf).body_goal_expr().body_goal_exprs().empty()) {
                open_leaves.push_back(leaf);
                continue;
            }
            if (!solution.has_value() || leaf < solution.value())
                solution = leaf;
        }
        if (solution.has_value()) {
            yielded_solutions.insert(solution.value());
            co_yield pud_solution{solution.value()};
            continue;
        }
        if (open_leaves.empty())
            co_return;
        const pud_decision decision = generate_decision_.generate(open_leaves);
        expand_leaf_.expand(decision.leaf, decision.goal);
        co_yield decision;
    }
}

#undef PUD_SOLVER
#undef PUD_SOLVER_TEMPLATE

#endif
