#ifndef PUD_LODESTAR_UNIT_GOAL_CHOOSER_HPP
#define PUD_LODESTAR_UNIT_GOAL_CHOOSER_HPP

#include <optional>
#include "value_objects/pud_body_goal_id.hpp"
#include "value_objects/pud_global_node_id.hpp"

// Chooses which goal of a leaf to unfold: a unit goal if the leaf holds one, since its unfold is forced once the
// leaf is chosen. Otherwise the choice is left to the other chooser.
template<typename IFindUnitGoal, typename IChooseGoal>
struct pud_lodestar_unit_goal_chooser {
    pud_lodestar_unit_goal_chooser(IFindUnitGoal& find_unit_goal, IChooseGoal& choose_goal);
    pud_body_goal_id choose(pud_global_node_id leaf);
private:
    IFindUnitGoal& find_unit_goal_;
    IChooseGoal& choose_goal_;
};

#define PUD_LODESTAR_UNIT_GOAL_CHOOSER_TEMPLATE template<typename IFindUnitGoal, typename IChooseGoal>
#define PUD_LODESTAR_UNIT_GOAL_CHOOSER          pud_lodestar_unit_goal_chooser<IFindUnitGoal, IChooseGoal>

PUD_LODESTAR_UNIT_GOAL_CHOOSER_TEMPLATE
PUD_LODESTAR_UNIT_GOAL_CHOOSER::pud_lodestar_unit_goal_chooser(IFindUnitGoal& find_unit_goal, IChooseGoal& choose_goal)
    : find_unit_goal_(find_unit_goal)
    , choose_goal_(choose_goal) {}

PUD_LODESTAR_UNIT_GOAL_CHOOSER_TEMPLATE
pud_body_goal_id PUD_LODESTAR_UNIT_GOAL_CHOOSER::choose(pud_global_node_id leaf) {
    const std::optional<pud_body_goal_id> unit_goal = find_unit_goal_.find_unit_goal(leaf);
    if (unit_goal.has_value())
        return unit_goal.value();
    return choose_goal_.choose(leaf);
}

#undef PUD_LODESTAR_UNIT_GOAL_CHOOSER
#undef PUD_LODESTAR_UNIT_GOAL_CHOOSER_TEMPLATE

#endif
