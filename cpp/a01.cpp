#include "../hpp/a01.hpp"
#include "../hpp/a01_sim.hpp"
#include "../hpp/a01_decider.hpp"

a01::~a01() {
    t.pop();
}

a01::a01(
    const a01_database& db,
    const a01_goals& goals,
    trail& t,
    sequencer& vars,
    expr_pool& ep,
    bind_map& bm,
    lineage_pool& lp,
    size_t max_resolutions,
    size_t iterations_per_avoidance,
    double c,
    std::mt19937& rng
) :
    db(db),
    goals(goals),
    t(t),
    vars(vars),
    ep(ep),
    bm(bm),
    lp(lp),
    max_resolutions(max_resolutions),
    iterations_per_avoidance(iterations_per_avoidance),
    c(c),
    rng(rng),
    as({})
{
    t.push();
}

bool a01::operator()(size_t iterations, std::optional<a01_resolution_store>& soln) {
    // default to no solution
    soln = std::nullopt;
    
    for (size_t i = 0; i < iterations; i++) {
        // trim the lineage pool between iterations
        lp.trim();
        
        // construct avoidance
        a01_decision_store avoidance;
    
        if (!next_avoidance(avoidance, soln))
            return false;

        // record the avoidance
        as.insert(avoidance);

        // pin the decisions
        for (const auto& rl : avoidance)
            lp.pin(rl);

        // check for solution
        if (soln.has_value()) {
            // pin the solution
            for (const auto& rl : soln.value())
                lp.pin(rl);
            break;
        }
    }
    
    return true;
}

bool a01::sim_one(monte_carlo::tree_node<a01_decider::choice>& root, a01_decision_store& ds, a01_resolution_store& rs) {
    // reset the trail
    t.pop();
    t.push();

    // construct the simulation
    monte_carlo::simulation<a01_decider::choice, std::mt19937> sim(root, c, rng);

    // construct the a01_sim
    a01_sim sim_instance(max_resolutions, db, goals, t, vars, ep, bm, lp, as, sim);

    // run the simulation
    bool sim_result = sim_instance();

    // export the decisions and resolutions
    ds = sim_instance.decisions();
    rs = sim_instance.resolutions();

    // terminate the simulation
    sim.terminate(-(double)ds.size());

    // return the result of the simulation
    return sim_result;

}

bool a01::next_avoidance(a01_decision_store& avoidance, std::optional<a01_resolution_store>& soln) {
    // default to no solution
    soln = std::nullopt;
    
    // construct the tree for finidng the next avoidance
    monte_carlo::tree_node<a01_decider::choice> root;

    // construct the decision and resolution stores
    a01_decision_store ds;
    a01_resolution_store rs;

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
