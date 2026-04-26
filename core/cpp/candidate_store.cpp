#include "../hpp/domain/entities/candidate_store/candidate_store.hpp"
#include "../hpp/infrastructure/locator.hpp"

candidate_store::candidate_store() :
    frontier<std::unordered_set<size_t>, candidate_expander>(
        locator::locate<database>(),
        locator::locate<lineage_pool>()
    )
{
    // get resources
    const database& db = locator::locate<database>();
    const goals& gl = locator::locate<goals>();
    lineage_pool& lp = locator::locate<lineage_pool>();
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
