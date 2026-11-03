#ifndef PUD_SOLVED_LEAF_FINDER_HPP
#define PUD_SOLVED_LEAF_FINDER_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// Finds the live leaves under the focus that are solved and unrefuted, reading the solved focus leaves kept
// as leaves go live instead of walking every live leaf.
template<typename IGetSolvedFocusLeaves, typename IIsNodeRefuted>
struct pud_solved_leaf_finder {
    pud_solved_leaf_finder(IGetSolvedFocusLeaves& get_solved_focus_leaves, IIsNodeRefuted& is_refuted);
    std::optional<pud_global_node_id> find() const;
    std::vector<pud_global_node_id> list() const;
private:
    IGetSolvedFocusLeaves& get_solved_focus_leaves_;
    IIsNodeRefuted& is_refuted_;
};

#define PUD_SOLVED_LEAF_FINDER_TEMPLATE template<typename IGetSolvedFocusLeaves, typename IIsNodeRefuted>
#define PUD_SOLVED_LEAF_FINDER          pud_solved_leaf_finder<IGetSolvedFocusLeaves, IIsNodeRefuted>

PUD_SOLVED_LEAF_FINDER_TEMPLATE
PUD_SOLVED_LEAF_FINDER::pud_solved_leaf_finder(
    IGetSolvedFocusLeaves& get_solved_focus_leaves, IIsNodeRefuted& is_refuted)
    : get_solved_focus_leaves_(get_solved_focus_leaves)
    , is_refuted_(is_refuted) {}

PUD_SOLVED_LEAF_FINDER_TEMPLATE
std::optional<pud_global_node_id> PUD_SOLVED_LEAF_FINDER::find() const {
    for (const pud_global_node_id leaf : get_solved_focus_leaves_.solved()) {
        if (is_refuted_.is_refuted(leaf))
            continue;
        return leaf;
    }
    return std::nullopt;
}

PUD_SOLVED_LEAF_FINDER_TEMPLATE
std::vector<pud_global_node_id> PUD_SOLVED_LEAF_FINDER::list() const {
    std::vector<pud_global_node_id> solved;
    for (const pud_global_node_id leaf : get_solved_focus_leaves_.solved()) {
        if (is_refuted_.is_refuted(leaf))
            continue;
        solved.push_back(leaf);
    }
    return solved;
}

#undef PUD_SOLVED_LEAF_FINDER
#undef PUD_SOLVED_LEAF_FINDER_TEMPLATE

#endif
