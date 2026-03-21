#include <stdexcept>
#include "../hpp/goal_resolver.hpp"

goal_resolver::goal_resolver(
    resolution_store& r,
    goal_store& g,
    candidate_store& c,
    const database& d,
    copier& copier,
    bind_map& b,
    lineage_pool& l,
    goal_adder& a)
    : rs(r), gs(g), cs(c), db(d), cp(copier), bm(b), lp(l), ga(a)
{
}

void goal_resolver::operator()(const goal_lineage* gl, size_t i)
{
    // get the goal expression
    const expr* goal_expr = gs.at(gl);

    // remove the goal from the goals store
    gs.erase(gl);

    // remove the candidate entries from the candidates store
    cs.erase(gl);

    // construct the resolution lineage
    const resolution_lineage* rl = lp.resolution(gl, i);

    // add the resolution to the resolutions store
    rs.insert(rl);

    // get the rule in the db
    const rule& r = db.at(i);

    // create the translation map for copying the rule
    std::map<uint32_t, uint32_t> translation_map;

    // copy the head of the rule
    const expr* copied_head = cp(r.head, translation_map);

    // copy the body of the rule
    std::vector<const expr*> copied_body;
    for (const expr* e : r.body)
        copied_body.push_back(cp(e, translation_map));

    // unify the head with the goal
    if (!bm.unify(copied_head, goal_expr))
        throw std::runtime_error("Failed to unify the head with the goal");

    // add the body expressions to the goal store
    for (size_t j = 0; j < copied_body.size(); ++j) {
        const expr* e = copied_body[j];
        const goal_lineage* child_gl = lp.goal(rl, j);
        ga(child_gl, e);
    }
}
