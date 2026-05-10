#ifndef MCTS_DECISION_GENERATOR_HPP
#define MCTS_DECISION_GENERATOR_HPP

#include <random>
#include "../domain/interfaces/i_decision_generator.hpp"
#include "../domain/interfaces/i_lineage_pool.hpp"
#include "../domain/interfaces/i_active_goal_store.hpp"
#include "../domain/interfaces/i_candidates_frontier.hpp"
#include "../../../mcts/include/mcts.hpp"
#include "mcts_choice.hpp"

struct mcts_decision_generator : i_decision_generator {
    mcts_decision_generator();
    const resolution_lineage* generate() override;
private:
    const goal_lineage* choose_goal();
    size_t choose_candidate(const goal_lineage*);

    i_lineage_pool& lp;
    const i_active_goal_store& ags;
    const i_candidates_frontier& gcs;
    monte_carlo::simulation<mcts_choice, std::mt19937>& sim;
};

#endif
