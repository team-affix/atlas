#ifndef A01_SIM_HPP
#define A01_SIM_HPP

#include "../../mcts/include/mcts.hpp"
#include "defs.hpp"
#include "../../common/hpp/mcts_decider.hpp"
#include "../../common/hpp/solution_detector.hpp"
#include "../../common/hpp/conflict_detector.hpp"
#include "../../common/hpp/head_elimination_detector.hpp"
#include "../../common/hpp/cdcl_elimination_detector.hpp"
#include "../../common/hpp/unit_propagation_detector.hpp"
#include "goal_adder.hpp"
#include "goal_resolver.hpp"

struct a01_sim {
    a01_sim(
        size_t,
        const a01_database&,
        const a01_goals&,
        trail&,
        sequencer&,
        expr_pool&,
        bind_map&,
        lineage_pool&,
        a01_resolution_store&,
        a01_decision_store&,
        a01_avoidance_store,
        monte_carlo::simulation<mcts_decider::choice, std::mt19937>&
    );
    bool operator()();
#ifndef DEBUG
private:
#endif
    size_t max_resolutions;

    const a01_database& db;
    trail& t;
    lineage_pool& lp;
    a01_resolution_store& rs;
    a01_decision_store& ds;
    
    a01_avoidance_store as_copy;

    a01_goal_store gs;
    a01_candidate_store cs;

    
    copier cp;

    solution_detector sd;
    conflict_detector cd;

    head_elimination_detector he;
    cdcl_elimination_detector ce;
    unit_propagation_detector up;

    mcts_decider dec;

    a01_goal_adder ga;
    a01_goal_resolver gr;
};

#endif
