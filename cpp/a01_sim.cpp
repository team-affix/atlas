#include "../hpp/a01_sim.hpp"

a01_sim::a01_sim(
    size_t max_resolutions,
    const database& db,
    const goals& goals,
    trail& t,
    sequencer& vars,
    expr_pool& ep,
    bind_map& bm,
    lineage_pool& lp,
    resolution_store& rs,
    decision_store& ds,
    avoidance_store as,
    avoidance_map am,
    monte_carlo::simulation<mcts_decider::choice, std::mt19937>& sim
) :
    max_resolutions(max_resolutions),
    db(db),
    t(t),
    lp(lp),
    rs(rs),
    ds(ds),
    as_copy(as),
    am_copy(am),
    gs({}),
    cs({}),
    cp(vars, ep),
    sd(gs),
    cd(gs, cs),
    he(t, bm, gs, db),
    ce(as_copy, lp),
    up(cs),
    dec(gs, cs, sim),
    ga(gs, cs, db),
    gr(rs, gs, cs, db, cp, bm, lp, ga),
    at(as_copy, am_copy)
{
    // clear the resolution and decision stores
    // for the start of the simulation
    rs.clear();
    ds.clear();

    size_t i = 0;
    for (const auto& goal : goals)
        ga(lp.goal(nullptr, i++), goal);
}

bool a01_sim::operator()() {

    while ((rs.size() < max_resolutions) && !cd() && !sd()) {

        // head elimination
        size_t elim0 = std::erase_if(cs, [this](const auto& e) { return he(e.first, e.second); });

        // cdcl elimination
        size_t elim1 = std::erase_if(cs, [this](const auto& e) { return ce(e.first, e.second); });
        
        // unit propagation
        const auto it = std::find_if(gs.begin(), gs.end(), [this](const auto& e) { return up(e.first); });

        // enact the propagation
        if (it != gs.end())
            gr(it->first, cs.find(it->first)->second);

        // continue until fixpoint
        if (elim0 > 0 || elim1 > 0 || it != gs.end())
            continue;

        // decide on a goal and candidate
        auto [chosen_goal, chosen_candidate] = dec();

        // resolve the chosen goal and candidate
        gr(chosen_goal, chosen_candidate);

        // construct the resolution lineage
        auto rl = lp.resolution(chosen_goal, chosen_candidate);

        // trim the avoidances concerning the resolution
        at(rl);
        
        // mark this resolution as a decision
        ds.insert(rl);

    }

    // return whether a solution was found
    return sd();
}
