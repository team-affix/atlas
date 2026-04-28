#include "../hpp/goal_store.hpp"
#include "../hpp/locator.hpp"
#include "../hpp/defs.hpp"

goal_store::goal_store()
    :
    frontier<const expr*, goal_expander>(),
    t(locator::locate<trail>(locator_keys::inst_trail)),
    cp(locator::locate<copier>(locator_keys::inst_copier)),
    bm(locator::locate<bind_map>(locator_keys::inst_bind_map)),
    lp(locator::locate<lineage_pool>(locator_keys::inst_lineage_pool))
{
    // get resources
    goals& gl = locator::locate<goals>(locator_keys::inst_goals);
    // add the goals to the frontier
    for (int i = 0; i < gl.size(); ++i)
        insert(lp.goal(nullptr, i), gl.at(i));
}

bool goal_store::applicable(const expr* const& e, const rule& r) {
    // push a temporary frame since bindings must be temporary
    t.push();

    // try to unify the head with the goal
    bool result = bm.unify(e, r.head);

    // pop the temporary frame
    t.pop();

    // return the result
    return result;
}

goal_expander goal_store::make_expander(const expr* const& e, const rule& r) {
    return goal_expander(e, r);
}
