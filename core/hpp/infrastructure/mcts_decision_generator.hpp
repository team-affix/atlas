#ifndef MCTS_DECISION_GENERATOR_HPP
#define MCTS_DECISION_GENERATOR_HPP

#include "infrastructure/locator.hpp"
#include "interfaces/i_generate_decision.hpp"
#include "interfaces/i_make_resolution_lineage.hpp"
#include "interfaces/i_iterate_root_goals.hpp"
#include "interfaces/i_iterate_child_goals.hpp"
#include "interfaces/i_mcts_choose.hpp"
#include "interfaces/i_get_goal_candidate_rule_ids.hpp"
#include "interfaces/i_is_active_goal.hpp"

struct mcts_decision_generator : i_generate_decision {
    mcts_decision_generator(locator& loc);
    const resolution_lineage* generate() override;
private:
    const goal_lineage* choose_goal();
    rule_id choose_candidate(const goal_lineage*);
    
    i_make_resolution_lineage& make_resolution_lineage;
    i_iterate_root_goals& iterate_root_goals;
    i_iterate_child_goals& iterate_child_goals;
    i_mcts_choose& mcts_choose;
    i_get_goal_candidate_rule_ids& get_goal_candidate_rule_ids;
    i_is_active_goal& is_active_goal;
};

#endif
