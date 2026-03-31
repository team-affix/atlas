#ifndef HORIZON_HPP
#define HORIZON_HPP

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

struct horizon {
    ~horizon();
    horizon(
        const database&,
        const goals&,
        trail&,
        sequencer&,
        bind_map&,
        size_t,
        double,
        std::mt19937&
    );
    bool operator()(size_t, std::optional<resolutions>&);
#ifndef DEBUG
private:
#endif
    bool sim_one(monte_carlo::tree_node<mcts_decider::choice>&, decisions&, resolutions&);
    
    const database& db;
    const goals& gl;
    trail& t;
    sequencer& vars;
    bind_map& bm;
    
    expr_pool ep;
    lineage_pool lp;
    
    size_t max_resolutions;
    double exploration_constant;
    std::mt19937& rng;

    cdcl c;
    monte_carlo::tree_node<mcts_decider::choice> root;
};

#endif
