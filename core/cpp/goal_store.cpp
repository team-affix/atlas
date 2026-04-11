#include <stdexcept>
#include "../hpp/goal_store.hpp"

goal_store::goal_store(
    const database& db,
    const goals& goals,
    trail& t,
    copier& cp,
    bind_map& bm,
    lineage_pool& lp)
    :
    frontier<const expr*>(db, lp, t),
    cp(cp),
    bm(bm)
{
    // add the goals to the frontier
    for (int i = 0; i < goals.size(); ++i)
        insert(lp.goal(nullptr, i), goals.at(i));
}

std::optional<std::vector<const expr*>> goal_store::expand(const expr* const& e, const rule& r) {
    // create the translation map for copying the rule
    std::map<uint32_t, uint32_t> translation_map;

    // copy the head of the rule
    const expr* copied_head = cp(r.head, translation_map);
    
    // unify the head with the goal
    if (!bm.unify(copied_head, e))
        return std::nullopt;

    // copy the body of the rule
    std::vector<const expr*> copied_body;
    for (const expr* e : r.body)
        copied_body.push_back(cp(e, translation_map));

    return copied_body;
}
