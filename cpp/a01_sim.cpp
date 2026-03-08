#include "../hpp/a01_sim.hpp"

a01_sim::a01_sim(
    const a01_database& db,
    trail& t,
    sequencer& vars,
    expr_pool& ep,
    bind_map& bm,
    lineage_pool& lp,
    monte_carlo::simulation<a01_decider::choice, std::mt19937>& sim,
    a01_goal_store gs,
    a01_candidate_store cs,
    a01_avoidance_store as
) :
    db(db),
    t(t),
    bm(bm),
    lp(lp),
    sim(sim),
    gs_copy(gs),
    cs_copy(cs),
    as_copy(as),
    rs({}),
    ds({}),
    cp(vars, ep),
    sd(gs_copy),
    cd(gs_copy, cs_copy),
    he(t, bm, gs_copy, db),
    ce(as_copy, lp),
    up(cs_copy),
    dec(gs_copy, cs_copy, sim),
    ga(gs_copy, cs_copy, db),
    gr(rs, gs_copy, cs_copy, db, cp, bm, lp, ga, as_copy)
{
}

bool a01_sim::operator()() {

    while (!cd() && !sd()) {

        // head elimination
        size_t elim0 = std::erase_if(cs_copy, [this](const auto& e) { return he(e.first, e.second); });

        // cdcl elimination
        size_t elim1 = std::erase_if(cs_copy, [this](const auto& e) { return ce(e.first, e.second); });
        
        // unit propagation
        const auto it = std::find_if(gs_copy.begin(), gs_copy.end(), [this](const auto& e) { return up(e.first); });

        // enact the propagation
        if (it != gs_copy.end())
            gr(it->first, cs_copy.find(it->first)->second);

        // continue until fixpoint
        if (elim0 > 0 || elim1 > 0 || it != gs_copy.end())
            continue;

        // decide on a goal and candidate
        auto [chosen_goal, chosen_candidate] = dec();

        // resolve the chosen goal and candidate
        gr(chosen_goal, chosen_candidate);

        // construct the resolution lineage
        auto rl = lp.resolution(chosen_goal, chosen_candidate);

        // mark this resolution as a decision
        ds.insert(rl);

    }

    // return whether a solution was found
    return sd();
}
