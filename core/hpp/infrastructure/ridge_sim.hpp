#ifndef RIDGE_SIM_HPP
#define RIDGE_SIM_HPP

#include "mcts_decider.hpp"
#include "sim.hpp"

struct ridge_sim : sim {
    ridge_sim();
#ifndef DEBUG
protected:
#endif
    const resolution_lineage* decide_one() override;
    void on_resolve(const resolution_lineage*) override;

    mcts_decider dec;
};

#endif
