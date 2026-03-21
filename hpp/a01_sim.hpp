#ifndef A01_SIM_HPP
#define A01_SIM_HPP

#include "../mcts/include/mcts.hpp"
#include "defs.hpp"
#include "mcts_decider.hpp"
#include "solution_detector.hpp"
#include "conflict_detector.hpp"
#include "head_elimination_detector.hpp"
#include "cdcl.hpp"
#include "unit_propagation_detector.hpp"
#include "goal_adder.hpp"
#include "goal_resolver.hpp"

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
        resolution_store&,
        decision_store&,
        cdcl c,
        monte_carlo::simulation<mcts_decider::choice, std::mt19937>&
    );
    bool operator()();
#ifndef DEBUG
private:
#endif
    size_t max_resolutions;

    const database& db;
    trail& t;
    lineage_pool& lp;
    resolution_store& rs;
    decision_store& ds;
    
    goal_store gs;
    candidate_store cs;
    
    
    copier cp;
    
    solution_detector sd;
    conflict_detector cd;
    
    head_elimination_detector he;
    unit_propagation_detector up;
    
    mcts_decider dec;
    
    goal_adder ga;
    goal_resolver gr;
    
    cdcl c;
};

#endif
