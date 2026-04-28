#include "../hpp/weight_store.hpp"
#include "../hpp/locator.hpp"

weight_store::weight_store() : frontier<double, weight_expander>(), cgw(0.0) {
    const goals& gl = locator::locate<goals>(locator_keys::inst_goals);
    lineage_pool& lpool = locator::locate<lineage_pool>(locator_keys::inst_lineage_pool);
    if (gl.size() == 0)
        return;
    double weight_per_goal = 1.0 / (double)gl.size();
    for (size_t i = 0; i < gl.size(); ++i)
        insert(lpool.goal(nullptr, i), weight_per_goal);
}

double weight_store::total() const {
    return cgw;
}

weight_expander weight_store::make_expander(const double& weight, const rule& r) {
    return weight_expander(weight, r, cgw);
}
