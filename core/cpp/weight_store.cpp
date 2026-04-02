#include "../hpp/weight_store.hpp"

weight_store::weight_store(
    const goals& goals,
    const database& db,
    lineage_pool& lp
) : frontier<double>(db, lp), cgw(0.0) {
    if (goals.size() == 0)
        return;
    double weight_per_goal = 1.0 / (double)goals.size();
    for (size_t i = 0; i < goals.size(); ++i)
        insert(lp.goal(nullptr, i), weight_per_goal);
}

double weight_store::total() const {
    return cgw;
}

std::vector<double> weight_store::expand(const double& weight, const rule& r) {
    std::vector<double> result;
    // if grounding against a fact, we receive the full weight
    if (r.body.size() == 0) {
        cgw += weight;
        return result;
    }
    // if resolving a non-nullary rule, we divide the weight equally among the children
    double child_weight = weight / (double)r.body.size();
    for (size_t i = 0; i < r.body.size(); ++i)
        result.push_back(child_weight);
    return result;
}
