#ifndef A01_SIM_HPP
#define A01_SIM_HPP

#include "../mcts/include/mcts.hpp"
#include "a01_defs.hpp"
#include "a01_decider.hpp"
#include "solution_detector.hpp"
#include "conflict_detector.hpp"
#include "a01_head_elimination_detector.hpp"
#include "a01_cdcl_elimination_detector.hpp"
#include "unit_propagation_detector.hpp"
#include "a01_goal_adder.hpp"
#include "a01_goal_resolver.hpp"

struct a01_sim {
    ~a01_sim();
    a01_sim(
        const a01_database&,
        const a01_goals&,
        trail&,
        sequencer&,
        expr_pool&,
        bind_map&,
        lineage_pool&,
        a01_avoidance_store,
        monte_carlo::simulation<a01_decider::choice, std::mt19937>&
    );
    bool operator()();
    const a01_decision_store& decisions() const;
#ifndef DEBUG
private:
#endif
    const a01_database& db;
    trail& t;
    lineage_pool& lp;
    
    a01_avoidance_store as_copy;

    a01_goal_store gs;
    a01_candidate_store cs;
    a01_resolution_store rs;
    a01_decision_store ds;

    
    copier cp;

    solution_detector sd;
    conflict_detector cd;

    a01_head_elimination_detector he;
    a01_cdcl_elimination_detector ce;
    unit_propagation_detector up;

    a01_decider dec;

    a01_goal_adder ga;
    a01_goal_resolver gr;
};

#endif
