#include "../hpp/candidate_store.hpp"
    
candidate_store::candidate_store(
    const database& db,
    const goals& goals,
    lineage_pool& lp) :
    frontier<std::unordered_set<size_t>>(db, lp),
    db(db),
    lp(lp)
{
    // make the initial candidates
    for (int i = 0; i < db.size(); ++i)
        initial_candidates.insert(i);
    // make the initial members
    for (int i = 0; i < goals.size(); ++i)
        insert(lp.goal(nullptr, i), initial_candidates);
}

bool candidate_store::conflicted() const {
    return std::any_of(
       begin(),
        end(),
        [](const auto& e) { return e.second.size() == 0; });
}

std::vector<std::unordered_set<size_t>> candidate_store::expand(const std::unordered_set<size_t>& candidates, const rule& r) {
    std::vector<std::unordered_set<size_t>> result;
    for (int i = 0; i < r.body.size(); ++i)
        result.push_back(initial_candidates);
    return result;
}
