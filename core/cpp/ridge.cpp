#include "../hpp/ridge.hpp"
#include "../hpp/ridge_sim.hpp"
#include "../hpp/mcts_decider.hpp"

ridge::~ridge() {
    t.pop();
}

ridge::ridge(
    const database& db,
    const goals& goals,
    trail& t,
    sequencer& vars,
    bind_map& bm,
    size_t max_resolutions,
    size_t iterations_per_avoidance,
    double exploration_constant,
    std::mt19937& rng
) :
    db(db),
    gl(goals),
    t(t),
    vars(vars),
    bm(bm),
    ep(t),
    lp(),
    max_resolutions(max_resolutions),
    iterations_per_avoidance(iterations_per_avoidance),
    exploration_constant(exploration_constant),
    rng(rng),
    c()
{
    t.push();
}

bool ridge::operator()(size_t iterations, std::optional<resolutions>& soln) {
    // default to no solution
    soln = std::nullopt;

    // if the a01 has already found the last solution, then it is refuted
    // this is required if the last solution found required no decisions
    if (c.refuted())
        return false;
    
    for (size_t i = 0; i < iterations; i++) {
        // trim the lineage pool between iterations
        lp.trim();
        
        // construct avoidance
        decisions ds;
    
        if (!next_avoidance(ds, soln))
            return false;

        // record the avoidance
        c.learn(ds);

        // pin the decisions
        for (const auto& rl : ds)
            lp.pin(rl);

        // check for solution
        if (soln.has_value())
            break;
    }
    
    return true;
}

bool ridge::sim_one(monte_carlo::tree_node<mcts_decider::choice>& root, decisions& ds, resolutions& rs) {
    // reset the trail
    t.pop();
    t.push();

    // construct the simulation
    monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, exploration_constant, rng);

    // construct the a01_sim
    ridge_sim sim_instance(max_resolutions, db, gl, t, vars, ep, bm, lp, c, sim);

    // run the simulation
    bool sim_result = sim_instance();
    
    // get the rs and ds
    rs = sim_instance.get_resolutions();
    ds = sim_instance.get_decisions();

    // terminate the simulation
    sim.terminate(-(double)ds.size());

    // return the result of the simulation
    return sim_result;

}

bool ridge::next_avoidance(decisions& avoidance, std::optional<resolutions>& soln) {
    // default to no solution
    soln = std::nullopt;
    
    // construct the tree for finidng the next avoidance
    monte_carlo::tree_node<mcts_decider::choice> root;

    // construct the decision and resolution stores
    decisions ds;
    resolutions rs;

    // perform refutation check
    if (!sim_one(root, ds, rs) && ds.empty())
        return false;

    // keep track of the smallest decision store
    avoidance = ds;
    
    for (size_t i = 0; i < iterations_per_avoidance; i++) {
        // simulate once
        if (sim_one(root, ds, rs)) {
            // on solution, export the avoidance and proof
            avoidance = ds;
            soln = rs;
            break;
        }

        // prefer smaller decision stores
        if (ds.size() < avoidance.size())
            avoidance = ds;
    }

    // no solution found
    return true;
}
