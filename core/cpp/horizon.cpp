#include "../hpp/horizon.hpp"
#include "../hpp/horizon_sim.hpp"
#include "../hpp/mcts_decider.hpp"

horizon::~horizon() {
    t.pop();
}

horizon::horizon(
    const database& db,
    const goals& goals,
    trail& t,
    sequencer& vars,
    bind_map& bm,
    size_t max_resolutions,
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
    exploration_constant(exploration_constant),
    rng(rng),
    c()
{
    t.push();
}

bool horizon::operator()(size_t iterations, std::optional<resolutions>& soln) {
    // default to no solution
    soln = std::nullopt;

    // if the solver has already found the last solution, then it is refuted
    // this is required if the last solution found required no decisions
    if (c.refuted())
        return false;
    
    for (size_t i = 0; i < iterations; i++) {
        // trim the lineage pool between iterations
        lp.trim();
        
        // construct avoidance
        resolutions rs;
        decisions ds;

        if (sim_one(root, ds, rs))
            // if we found a solution, then we yield it
            soln = rs;
        else if (ds.empty())
            // if we reached a conflict with NO decisions, then we are refuted
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

bool horizon::sim_one(monte_carlo::tree_node<mcts_decider::choice>& root, decisions& ds, resolutions& rs) {
    // reset the trail
    t.pop();
    t.push();

    // construct the simulation
    monte_carlo::simulation<mcts_decider::choice, std::mt19937> sim(root, exploration_constant, rng);

    // construct the a01_sim
    horizon_sim sim_instance(max_resolutions, db, gl, t, vars, ep, bm, lp, c, sim);

    // run the simulation
    bool sim_result = sim_instance();
    
    // get the rs and ds
    rs = sim_instance.get_resolutions();
    ds = sim_instance.get_decisions();

    // terminate the simulation
    sim.terminate(sim_instance.reward());

    // return the result of the simulation
    return sim_result;

}
