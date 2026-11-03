#ifndef PUD_STORED_SOLUTION_DROPPER_HPP
#define PUD_STORED_SOLUTION_DROPPER_HPP

#include <optional>
#include <vector>
#include "value_objects/pud_global_node_id.hpp"

// After a module is loaded, takes the solved focus leaves an earlier session already derived off the
// frontier without reporting them, so a session over saved modules reports only solutions it finds
// itself. A solved leaf counts as earlier-derived when it is an inference (it has a parent): the focus
// axiom itself, if it is a fact, was never derived and is always reported. Does nothing when stored
// solutions are to be reported again. Nothing is written to the modules.
template<typename IListSolvedLeaves, typename IGetNodeParent, typename IEraseFrontierNode>
struct pud_stored_solution_dropper {
    pud_stored_solution_dropper(
        IListSolvedLeaves& list_solved_leaves,
        IGetNodeParent& get_node_parent,
        IEraseFrontierNode& erase_frontier_node,
        bool report_stored_solutions);
    void drop();
private:
    IListSolvedLeaves& list_solved_leaves_;
    IGetNodeParent& get_node_parent_;
    IEraseFrontierNode& erase_frontier_node_;
    bool report_stored_solutions_;
};

#define PUD_STORED_SOLUTION_DROPPER_TEMPLATE \
    template<typename IListSolvedLeaves, typename IGetNodeParent, typename IEraseFrontierNode>
#define PUD_STORED_SOLUTION_DROPPER \
    pud_stored_solution_dropper<IListSolvedLeaves, IGetNodeParent, IEraseFrontierNode>

PUD_STORED_SOLUTION_DROPPER_TEMPLATE
PUD_STORED_SOLUTION_DROPPER::pud_stored_solution_dropper(
    IListSolvedLeaves& list_solved_leaves,
    IGetNodeParent& get_node_parent,
    IEraseFrontierNode& erase_frontier_node,
    bool report_stored_solutions)
    : list_solved_leaves_(list_solved_leaves)
    , get_node_parent_(get_node_parent)
    , erase_frontier_node_(erase_frontier_node)
    , report_stored_solutions_(report_stored_solutions) {}

PUD_STORED_SOLUTION_DROPPER_TEMPLATE
void PUD_STORED_SOLUTION_DROPPER::drop() {
    if (report_stored_solutions_)
        return;
    for (const pud_global_node_id leaf : list_solved_leaves_.list()) {
        const bool derived = get_node_parent_.get_parent(leaf).has_value();
        if (!derived)
            continue;
        erase_frontier_node_.erase(leaf);
    }
}

#undef PUD_STORED_SOLUTION_DROPPER
#undef PUD_STORED_SOLUTION_DROPPER_TEMPLATE

#endif
