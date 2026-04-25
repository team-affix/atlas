#include "../hpp/candidate_store.hpp"
    
candidate_store::candidate_store(
    const database& db,
    const goals& goals,
    lineage_pool& lp) :
    frontier<std::unordered_set<size_t>, candidate_expander>(db, lp)
{
    // make the initial candidates
    for (int i = 0; i < db.size(); ++i)
        initial_candidates.insert(i);
    // make the initial members
    for (int i = 0; i < goals.size(); ++i)
        insert(lp.goal(nullptr, i), initial_candidates);
}

candidate_expander candidate_store::make_expander(const std::unordered_set<size_t>&, const rule&) {
    return candidate_expander(initial_candidates);
}
