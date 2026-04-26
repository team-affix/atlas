#ifndef HORIZON_SIM_HPP
#define HORIZON_SIM_HPP

#include "mcts_decider.hpp"
#include "sim.hpp"
#include "weight_store.hpp"

struct horizon_sim : sim {
    horizon_sim();
    double reward();
#ifndef DEBUG
protected:
#endif
    const resolution_lineage* decide_one() override;
    void on_resolve(const resolution_lineage*) override;

    mcts_decider dec;
    weight_store ws;
};

#endif
