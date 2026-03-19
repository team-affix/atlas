#ifndef A01_HPP
#define A01_HPP

#include <optional>
#include <random>
#include "../../common/hpp/trail.hpp"
#include "../../common/hpp/expr.hpp"
#include "../../common/hpp/bind_map.hpp"
#include "../../common/hpp/lineage.hpp"
#include "../../common/hpp/sequencer.hpp"
#include "defs.hpp"
#include "../../common/hpp/mcts_decider.hpp"
#include "../../mcts/include/mcts.hpp"

struct a01 {
    ~a01();
    a01(
        const a01_database&,
        const a01_goals&,
        trail&,
        sequencer&,
        bind_map&,
        size_t,
        size_t,
        double,
        std::mt19937&
    );
    bool operator()(size_t, std::optional<a01_resolution_store>&);
#ifndef DEBUG
private:
#endif
    bool sim_one(monte_carlo::tree_node<mcts_decider::choice>&, a01_decision_store&, a01_resolution_store&);
    bool next_avoidance(a01_decision_store&, std::optional<a01_resolution_store>&);
    
    const a01_database& db;
    const a01_goals& goals;
    trail& t;
    sequencer& vars;
    bind_map& bm;
    
    expr_pool ep;
    lineage_pool lp;
    
    size_t max_resolutions;
    size_t iterations_per_avoidance;
    double c;
    std::mt19937& rng;

    a01_avoidance_store as;
};

#endif
