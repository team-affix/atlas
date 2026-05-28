#ifndef MCTS_DECISION_GENERATOR_HPP
#define MCTS_DECISION_GENERATOR_HPP

#include <random>
#include "../interfaces/i_generate_decision.hpp"
#include "../interfaces/i_make_resolution_lineage.hpp"
#include "../interfaces/i_iterate_active_goals.hpp"
#include "../interfaces/i_active_goals_size.hpp"
#include "../interfaces/i_get_goal_candidate_rules.hpp"
#include "../../../mcts/include/mcts.hpp"
#include "../value_objects/mcts_choice.hpp"

struct mcts_decision_generator : i_generate_decision {
    mcts_decision_generator(
        i_make_resolution_lineage& make_resolution_lineage,
        i_iterate_active_goals& iterate_active_goals,
        i_active_goals_size& active_goals_size,
        i_get_goal_candidate_rules& ggcr,
        monte_carlo::simulation<mcts_choice, std::mt19937>& sim);
    const resolution_lineage* generate() override;
private:
    const goal_lineage* choose_goal();
    rule_id choose_candidate(const goal_lineage*);

    i_make_resolution_lineage& make_resolution_lineage;
    i_iterate_active_goals& iterate_active_goals;
    i_active_goals_size& active_goals_size;
    i_get_goal_candidate_rules& ggcr;
    monte_carlo::simulation<mcts_choice, std::mt19937>& sim;
};

#endif
