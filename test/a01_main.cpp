#include <stdexcept>
#include "../hpp/a01_defs.hpp"
#include "../hpp/a01_goal_resolver.hpp"
#include "../hpp/a01_head_elimination_detector.hpp"
#include "../hpp/unit_propagation_detector.hpp"
#include "../hpp/solution_detector.hpp"
#include "../hpp/conflict_detector.hpp"
#include "../hpp/a01_cdcl_elimination_detector.hpp"
#include "../hpp/a01_decider.hpp"
#include "../hpp/normalizer.hpp"

bool a01_sim_one(
    lineage_pool& lp,
    a01_goal_store& gs,
    a01_candidate_store& cs,
    a01_decision_store& ds,
    a01_resolution_store& rs,
    conflict_detector& cd,
    solution_detector& sd,
    a01_head_elimination_detector& he,
    a01_cdcl_elimination_detector& ce,
    unit_propagation_detector& up,
    a01_goal_resolver& gr,
    a01_decider& dec) {
    
    while (!cd() && !sd()) {
        size_t elim0 = std::erase_if(cs, [&he](const auto& e) { return he(e.first, e.second); });
        size_t elim1 = std::erase_if(cs, [&ce](const auto& e) { return ce(e.first, e.second); });
        const auto it = std::find_if(gs.begin(), gs.end(), [&up](const auto& e) { return up(e.first); });

        // continue until fixpoint
        if (elim0 > 0 || elim1 > 0 || it != gs.end())
            continue;

        auto [chosen_goal, chosen_candidate] = dec();

        gr(chosen_goal, chosen_candidate);

        auto rl = lp.resolution(chosen_goal, chosen_candidate);

        ds.insert(rl);
        rs.insert(rl);

    }

    return sd();
    
}

void init_ep_0(expr_pool& ep, lineage_pool& lp, a01_database& db, a01_goal_store& gs, a01_candidate_store& cs) {
    // edit database
    db.push_back(rule{ep.atom("a"), {}});

    // construct goal adder
    a01_goal_adder ga(gs, cs, db);

    // edit goal store
    ga(lp.goal(nullptr, 0), ep.atom("a"));
}

void a01() {
    trail t;
    bind_map bm(t);
    sequencer vars(t);
    expr_pool ep(t);
    copier cp(vars, ep);
    normalizer n(ep, bm);
    lineage_pool lp;
    
    a01_goal_store gs_main;
    a01_goal_store gs_working_copy;
    a01_candidate_store cs_main;
    a01_candidate_store cs_working_copy;
    a01_resolution_store rs_main;
    a01_resolution_store rs_working_copy;
    a01_decision_store ds_main;
    a01_decision_store ds_working_copy;
    a01_avoidance_store as_main;
    a01_avoidance_store as_working_copy;
    a01_database db;

    solution_detector sd(gs_working_copy);
    conflict_detector cd(gs_working_copy, cs_working_copy);
    a01_head_elimination_detector he(t, bm, gs_working_copy, db);
    a01_cdcl_elimination_detector ce(as_working_copy, lp);
    unit_propagation_detector up(cs_working_copy);
    a01_goal_adder ga(gs_working_copy, cs_working_copy, db);
    a01_goal_resolver gr(rs_working_copy, gs_working_copy, cs_working_copy, db, cp, bm, lp, ga, as_working_copy);

    // PUSH INITIAL FRAME
    t.push();

    // CHOOSE EXAMPLE PROBLEM
    init_ep_0(ep, lp, db, gs_main, cs_main);
    
    constexpr size_t ITERATIONS_BEFORE_CDCL = 1000;
    constexpr double EXPLORATION_CONSTANT = 1.414;
    
    while(true) {
        monte_carlo::tree_node<a01_decider::choice> root;

        a01_decision_store smallest_ds;
        size_t smallest_ds_size = std::numeric_limits<size_t>::max();
        
        for (size_t i = 0; i < ITERATIONS_BEFORE_CDCL; ++i) {

            std::random_device rd;
            std::mt19937 rng(rd());

            monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, EXPLORATION_CONSTANT, rng);

            a01_decider dec(gs_working_copy, cs_working_copy, sim);

            // reset all working copies
            gs_working_copy = gs_main;
            cs_working_copy = cs_main;
            rs_working_copy = {};
            ds_working_copy = {};
            as_working_copy = as_main;

            t.push();

            if (a01_sim_one(lp, gs_working_copy, cs_working_copy, ds_working_copy, rs_working_copy, cd, sd, he, ce, up, gr, dec)) {
                throw std::runtime_error("Solution found");
            }

            t.pop();

            // prefer smaller decision stores
            sim.terminate(-ds_working_copy.size());

            if (ds_working_copy.size() < smallest_ds_size) {
                smallest_ds = ds_working_copy;
                smallest_ds_size = ds_working_copy.size();
            }
        }

        // CDCL learning
        as_main.insert(smallest_ds);

        // trim (not yet implmented)
        
    }
    
}

#ifndef DEBUG

int main() {
    a01();
    return 0;
}

#endif
