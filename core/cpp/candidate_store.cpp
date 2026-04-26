#include "../hpp/candidate_store.hpp"
    
candidate_store::candidate_store() :
    frontier<std::unordered_set<size_t>, candidate_expander>()
{
    // get resources
    const database& db = locator::locate<database>(locator_keys::inst_database);
    const goals& gl = locator::locate<goals>(locator_keys::inst_goals);
    lineage_pool& lp = locator::locate<lineage_pool>(locator_keys::inst_lineage_pool);
    // make the initial candidates
    for (int i = 0; i < db.size(); ++i)
        initial_candidates.insert(i);
    // make the initial members
    for (int i = 0; i < gl.size(); ++i)
        insert(lp.goal(nullptr, i), initial_candidates);
}

candidate_expander candidate_store::make_expander(const std::unordered_set<size_t>&, const rule&) {
    return candidate_expander(initial_candidates);
}
