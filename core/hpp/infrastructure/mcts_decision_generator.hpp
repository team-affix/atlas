#ifndef MCTS_DECISION_GENERATOR_HPP
#define MCTS_DECISION_GENERATOR_HPP

#include <random>
#include "../interfaces/i_decision_generator.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_active_goals.hpp"
#include "../interfaces/i_get_goal_candidate_rules.hpp"
#include "../../../mcts/include/mcts.hpp"
#include "../value_objects/mcts_choice.hpp"

struct mcts_decision_generator : i_decision_generator {
    mcts_decision_generator(
        i_lineage_pool& lp,
        const i_active_goals& ag,
        i_get_goal_candidate_rules& ggcr,
        monte_carlo::simulation<mcts_choice, std::mt19937>& sim);
    const resolution_lineage* generate() override;
private:
    const goal_lineage* choose_goal();
    const rule* choose_candidate(const goal_lineage*);

    i_lineage_pool& lp;
    const i_active_goals& ag;
    i_get_goal_candidate_rules& ggcr;
    monte_carlo::simulation<mcts_choice, std::mt19937>& sim;
};

#endif
