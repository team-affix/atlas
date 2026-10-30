#ifndef PUD_LODESTAR_SOLUTION_TAKER_HPP
#define PUD_LODESTAR_SOLUTION_TAKER_HPP

#include <optional>
#include "value_objects/expr.hpp"
#include "value_objects/pud_global_node_id.hpp"
#include "value_objects/pud_solution_term.hpp"

template<
    typename IFindSolvedLeaf,
    typename IReadSolutionTerm,
    typename INormalizeSolution,
    typename ICountNodeDepth,
    typename ISetSolution,
    typename IEraseFrontierNode>
struct pud_lodestar_solution_taker {
    pud_lodestar_solution_taker(
        IFindSolvedLeaf& find_solved_leaf,
        IReadSolutionTerm& read_solution_term,
        INormalizeSolution& normalize_solution,
        ICountNodeDepth& count_node_depth,
        ISetSolution& set_solution,
        IEraseFrontierNode& erase_frontier_node);
    bool take();
private:
    IFindSolvedLeaf& find_solved_leaf_;
    IReadSolutionTerm& read_solution_term_;
    INormalizeSolution& normalize_solution_;
    ICountNodeDepth& count_node_depth_;
    ISetSolution& set_solution_;
    IEraseFrontierNode& erase_frontier_node_;
};

#define PUD_LODESTAR_SOLUTION_TAKER_TEMPLATE                                                         \
    template<typename IFindSolvedLeaf, typename IReadSolutionTerm, typename INormalizeSolution,     \
             typename ICountNodeDepth, typename ISetSolution, typename IEraseFrontierNode>
#define PUD_LODESTAR_SOLUTION_TAKER                                                                  \
    pud_lodestar_solution_taker<IFindSolvedLeaf, IReadSolutionTerm, INormalizeSolution, ICountNodeDepth, \
        ISetSolution, IEraseFrontierNode>

PUD_LODESTAR_SOLUTION_TAKER_TEMPLATE
PUD_LODESTAR_SOLUTION_TAKER::pud_lodestar_solution_taker(
    IFindSolvedLeaf& find_solved_leaf,
    IReadSolutionTerm& read_solution_term,
    INormalizeSolution& normalize_solution,
    ICountNodeDepth& count_node_depth,
    ISetSolution& set_solution,
    IEraseFrontierNode& erase_frontier_node)
    : find_solved_leaf_(find_solved_leaf)
    , read_solution_term_(read_solution_term)
    , normalize_solution_(normalize_solution)
    , count_node_depth_(count_node_depth)
    , set_solution_(set_solution)
    , erase_frontier_node_(erase_frontier_node) {}

PUD_LODESTAR_SOLUTION_TAKER_TEMPLATE
bool PUD_LODESTAR_SOLUTION_TAKER::take() {
    const std::optional<pud_global_node_id> leaf = find_solved_leaf_.find();
    if (!leaf.has_value())
        return false;
    const pud_solution_term term = read_solution_term_.read(leaf.value());
    const expr* solution = normalize_solution_.normalize(term.head, term.module, term.bindings);
    set_solution_.set(solution, count_node_depth_.count(leaf.value()));
    erase_frontier_node_.erase(leaf.value());
    return true;
}

#undef PUD_LODESTAR_SOLUTION_TAKER
#undef PUD_LODESTAR_SOLUTION_TAKER_TEMPLATE

#endif
