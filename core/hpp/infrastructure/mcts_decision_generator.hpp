#ifndef MCTS_DECISION_GENERATOR_HPP
#define MCTS_DECISION_GENERATOR_HPP

#include <random>
#include "../interfaces/i_decision_generator.hpp"
#include "../interfaces/i_lineage_pool.hpp"
#include "../interfaces/i_active_goals.hpp"
#include "../interfaces/i_goal_candidates_acceptor.hpp"
#include "../../../mcts/include/mcts.hpp"
#include "../value_objects/mcts_choice.hpp"

struct mcts_decision_generator : i_decision_generator {
    mcts_decision_generator();
    const resolution_lineage* generate() override;
private:
    const goal_lineage* choose_goal();
    size_t choose_candidate(const goal_lineage*);

    i_lineage_pool& lp;
    const i_active_goals& ag;
    i_goal_candidates_acceptor& gca;
    monte_carlo::simulation<mcts_choice, std::mt19937>& sim;
};

#endif
