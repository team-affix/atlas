#ifndef A01_SIM_HPP
#define A01_SIM_HPP

#include "../mcts/include/mcts.hpp"
#include "defs.hpp"
#include "mcts_decider.hpp"
#include "head_elimination_detector.hpp"
#include "cdcl.hpp"

struct a01_sim {
    a01_sim(
        size_t,
        const database&,
        const goals&,
        trail&,
        sequencer&,
        expr_pool&,
        bind_map&,
        lineage_pool&,
        resolutions&,
        decisions&,
        cdcl c,
        monte_carlo::simulation<mcts_decider::choice, std::mt19937>&
    );
    bool operator()();
#ifndef DEBUG
private:
#endif
    void add(const goal_lineage*, const expr*);
    void resolve(const resolution_lineage*);

    size_t max_resolutions;

    const database& db;
    trail& t;
    lineage_pool& lp;
    resolutions& rs;
    decisions& ds;
    
    goal_store gs;
    candidate_store cs;
    
    copier cp;
    
    head_elimination_detector he;
    
    mcts_decider dec;
    
    cdcl c;
};

#endif
