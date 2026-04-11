#include "../hpp/candidate_store.hpp"
    
candidate_store::candidate_store(
    const database& db,
    const goals& goals,
    lineage_pool& lp,
    trail& t) :
    frontier<std::vector<size_t>>(db, lp, t)
{
    // make the initial candidates
    for (int i = 0; i < db.size(); ++i)
        initial_candidates.push_back(i);
    // make the initial members
    for (int i = 0; i < goals.size(); ++i)
        insert(lp.goal(nullptr, i), initial_candidates);
}

size_t candidate_store::eliminate(const std::function<bool(const goal_lineage*, size_t)>& pred) {
    size_t result = 0;
    for (auto it = get().begin(); it != get().end(); ++it) {
        const goal_lineage* gl = it->first;
        std::vector<size_t> candidates = it->second;
        for (size_t i = 0; i < candidates.size();) {
            if (pred(gl, candidates[i])) {
                candidates[i] = candidates.back();
                candidates.pop_back();
                ++result;
            }
            else {
                ++i;
            }
        }
        assign(gl, candidates);
    }
    return result;
}

bool candidate_store::unit(const goal_lineage*& gl, size_t& candidate) const {
    for (auto it = get().begin(); it != get().end(); ++it) {
        const goal_lineage* key = it->first;
        const std::vector<size_t>& candidates = it->second;
        if (candidates.size() == 1) {
            gl = key;
            candidate = candidates.front();
            return true;
        }
    }
    return false;
}

bool candidate_store::conflicted() const {
    return std::any_of(
       get().begin(),
        get().end(),
        [](const auto& e) { return e.second.size() == 0; });
}

std::optional<std::vector<std::vector<size_t>>> candidate_store::expand(const std::vector<size_t>&, const rule& r) {
    return std::vector<std::vector<size_t>>(r.body.size(), initial_candidates);
}
