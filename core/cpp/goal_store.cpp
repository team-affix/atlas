#include <stdexcept>
#include "../hpp/goal_store.hpp"

goal_store::goal_store(
    const database& db,
    const goals& goals,
    trail& t,
    copier& cp,
    bind_map& bm,
    lineage_pool& lp,
    expr_pool& ep)
    :
    frontier<const expr::pred*>(db, lp),
    db(db),
    t(t),
    cp(cp),
    bm(bm),
    lp(lp),
    ep(ep)
{
    for (int i = 0; i < goals.size(); ++i)
        insert(lp.goal(nullptr, i), goals.at(i));
}

bool goal_store::try_unify_head(const expr::pred* const& goal, const rule& r, std::map<uint32_t, uint32_t>& translation_map) {
    const expr* copied_head = cp(ep.as_expr(r.head), translation_map);
    return bm.unify(copied_head, ep.as_expr(goal));
}

bool goal_store::applicable(const expr::pred* const& goal, const rule& r) {
    t.push();
    std::map<uint32_t, uint32_t> translation_map;
    bool result = try_unify_head(goal, r, translation_map);
    t.pop();
    return result;
}

std::vector<const expr::pred*> goal_store::expand(const expr::pred* const& goal, const rule& r) {
    std::map<uint32_t, uint32_t> translation_map;
    if (!try_unify_head(goal, r, translation_map))
        throw std::runtime_error("Failed to unify the head with the goal");

    std::vector<const expr::pred*> copied_body;
    for (const expr::pred* p : r.body) {
        const expr* copied = cp(ep.as_expr(p), translation_map);
        copied_body.push_back(&std::get<expr::pred>(copied->content));
    }
    return copied_body;
}
