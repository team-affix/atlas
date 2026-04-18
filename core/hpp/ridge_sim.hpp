#ifndef RIDGE_SIM_HPP
#define RIDGE_SIM_HPP

#include "../../mcts/include/mcts.hpp"
#include "mcts_decider.hpp"
#include "sim.hpp"

struct ridge_sim : sim {
    ridge_sim(
        size_t,
        const database&,
        const goals&,
        trail&,
        sequencer&,
        expr_pool&,
        bind_map&,
        lineage_pool&,
        cdcl,
        monte_carlo::simulation<mcts_decider::choice, std::mt19937>&
    );
#ifndef DEBUG
protected:
#endif
    const resolution_lineage* decide_one() override;

    mcts_decider dec;
};

#endif
