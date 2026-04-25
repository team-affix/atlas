#include "../hpp/weight_store.hpp"

weight_store::weight_store(
    const goals& goals,
    const database& db,
    lineage_pool& lp
) : frontier<double, weight_expander>(db, lp), cgw(0.0) {
    if (goals.size() == 0)
        return;
    double weight_per_goal = 1.0 / (double)goals.size();
    for (size_t i = 0; i < goals.size(); ++i)
        insert(lp.goal(nullptr, i), weight_per_goal);
}

double weight_store::total() const {
    return cgw;
}

weight_expander weight_store::make_expander(const double& weight, const rule& r) {
    return weight_expander(weight, r, cgw);
}
