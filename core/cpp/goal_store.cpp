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
    frontier<const expr*>(db, lp),
    db(db),
    t(t),
    cp(cp),
    bm(bm),
    lp(lp)
{
    // add the goals to the frontier
    for (int i = 0; i < goals.size(); ++i)
        insert(lp.goal(nullptr, i), goals.at(i));
}

bool goal_store::try_unify_head(const expr* const& e, const rule& r, std::map<uint32_t, uint32_t>& translation_map) {
    // copy the head of the rule
    const expr* copied_head = cp(r.head, translation_map);

    // unify the head with the goal
    return bm.unify(copied_head, e);
}

bool goal_store::applicable(const expr* const& e, const rule& r) {
    // push a temporary frame since bindings must be temporary
    t.push();

    // create the translation map for copying the rule
    std::map<uint32_t, uint32_t> translation_map;

    // try to unify the head with the goal
    bool applicable = try_unify_head(e, r, translation_map);

    // pop the temporary frame
    t.pop();

    // return the result
    return applicable;
}

std::vector<const expr*> goal_store::expand(const expr* const& e, const rule& r) {
    // create the translation map for copying the rule
    std::map<uint32_t, uint32_t> translation_map;

    // try to unify the head with the goal
    if (!try_unify_head(e, r, translation_map))
        throw std::runtime_error("Failed to unify the head with the goal");

    // copy the body of the rule
    std::vector<const expr*> copied_body;
    for (const expr* e : r.body)
        copied_body.push_back(cp(e, translation_map));

    return copied_body;
}
