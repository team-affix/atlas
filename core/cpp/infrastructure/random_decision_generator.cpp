#include "infrastructure/random_decision_generator.hpp"

random_decision_generator::random_decision_generator(locator& loc, std::mt19937& rng)
    :
    make_resolution_lineage(loc.locate<i_make_resolution_lineage>()),
    goal_random_access(loc.locate<i_random_access<const goal_lineage*>>()),
    active_goals_size(loc.locate<i_active_goals_size>()),
    get_goal_candidate_rule_ids(loc.locate<i_get_goal_candidate_rule_ids>()),
    rng(rng) {}

const resolution_lineage* random_decision_generator::generate() {
    const goal_lineage* gl = choose_goal();
    rule_id r = choose_candidate(gl);
    return make_resolution_lineage.make_resolution_lineage(gl, r);
}

const goal_lineage* random_decision_generator::choose_goal() {
    size_t n = active_goals_size.active_goals_size();
    std::uniform_int_distribution<size_t> dist(0, n - 1);
    return goal_random_access.select(dist(rng));
}

rule_id random_decision_generator::choose_candidate(const goal_lineage* gl) {
    i_rule_id_set& rules = get_goal_candidate_rule_ids.get(gl);
    auto& ra = static_cast<i_ra_rule_id_set&>(rules);
    std::uniform_int_distribution<size_t> dist(0, rules.size() - 1);
    return ra.select(dist(rng));
}
