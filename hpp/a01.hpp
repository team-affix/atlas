#ifndef A01_HPP
#define A01_HPP

#include <optional>
#include <random>
#include "trail.hpp"
#include "expr.hpp"
#include "bind_map.hpp"
#include "lineage.hpp"
#include "sequencer.hpp"
#include "defs.hpp"
#include "mcts_decider.hpp"
#include "cdcl.hpp"
#include "../mcts/include/mcts.hpp"

struct a01 {
    ~a01();
    a01(
        const database&,
        const goals&,
        trail&,
        sequencer&,
        bind_map&,
        size_t,
        size_t,
        double,
        std::mt19937&
    );
    bool operator()(size_t, std::optional<resolution_store>&);
#ifndef DEBUG
private:
#endif
    bool sim_one(monte_carlo::tree_node<mcts_decider::choice>&, decision_store&, resolution_store&);
    bool next_avoidance(decision_store&, std::optional<resolution_store>&);
    
    const database& db;
    const goals& gl;
    trail& t;
    sequencer& vars;
    bind_map& bm;
    
    expr_pool ep;
    lineage_pool lp;
    
    size_t max_resolutions;
    size_t iterations_per_avoidance;
    double exploration_constant;
    std::mt19937& rng;

    cdcl c;
};

#endif
