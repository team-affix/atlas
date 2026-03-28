#include "../hpp/weight_store.hpp"

weight_store::weight_store(
    const goals& goals,
    const database& db,
    lineage_pool& lp
) : frontier<double>(db, lp) {
    double weight_per_goal = 1.0 / (double)goals.size();
    for (size_t i = 0; i < goals.size(); ++i)
        insert(lp.goal(nullptr, i), weight_per_goal);
}

std::vector<double> weight_store::expand(const double& weight, const rule& r) {
    double child_weight = weight / (double)r.body.size();
    std::vector<double> result;
    for (size_t i = 0; i < r.body.size(); ++i)
        result.push_back(child_weight);
    return result;
}
