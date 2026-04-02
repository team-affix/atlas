#ifndef HORIZON_SIM_HPP
#define HORIZON_SIM_HPP

#include "ridge_sim.hpp"
#include "weight_store.hpp"

struct horizon_sim : ridge_sim {
    horizon_sim(
        size_t,
        const database&,
        const goals&,
        trail&,
        sequencer&,
        expr_pool&,
        bind_map&,
        lineage_pool&,
        cdcl c,
        monte_carlo::simulation<mcts_decider::choice, std::mt19937>&
    );
    double reward();
    void on_resolve(const resolution_lineage*) override;
#ifndef DEBUG
private:
#endif

    weight_store ws;
};

#endif
