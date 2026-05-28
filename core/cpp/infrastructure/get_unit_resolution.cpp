#include "infrastructure/get_unit_resolution.hpp"

get_unit_resolution::get_unit_resolution(locator& loc)
    :
    get_goal_candidate_rule_ids(loc.locate<i_get_goal_candidate_rule_ids>()),
    make_resolution_lineage(loc.locate<i_make_resolution_lineage>()) {}

const resolution_lineage* get_unit_resolution::get(const goal_lineage* gl) {
    auto& candidate_rules = get_goal_candidate_rule_ids.get(gl);
    auto it = candidate_rules.iterate();
    while (!it.done()) {
        auto rr = it.resume();
        if (!rr.has_value())
            continue;
        return make_resolution_lineage.make_resolution_lineage(gl, rr.value());
    }
    return nullptr;
}
