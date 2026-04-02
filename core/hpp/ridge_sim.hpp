#ifndef RIDGE_SIM_HPP
#define RIDGE_SIM_HPP

#include "../../mcts/include/mcts.hpp"
#include "defs.hpp"
#include "sequencer.hpp"
#include "bind_map.hpp"
#include "goal_store.hpp"
#include "candidate_store.hpp"
#include "mcts_decider.hpp"
#include "cdcl.hpp"
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
        cdcl c,
        monte_carlo::simulation<mcts_decider::choice, std::mt19937>&
    );
#ifndef DEBUG
private:
#endif
    bool solved() override;
    bool conflicted() override;
    const resolution_lineage* derive_one() override;
    const resolution_lineage* decide_one() override;
    void on_resolve(const resolution_lineage*) override;

    const database& db;
    trail& t;
    lineage_pool& lp;
    
    goal_store gs;
    candidate_store cs;
    
    copier cp;
    
    mcts_decider dec;
    
    cdcl c;
};

#endif
