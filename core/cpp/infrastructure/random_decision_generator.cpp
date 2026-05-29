#include "infrastructure/random_decision_generator.hpp"

random_decision_generator::random_decision_generator(locator& loc, std::mt19937& rng)
    :
    make_resolution_lineage(loc.locate<i_make_resolution_lineage>()),
    iterate_active_goals(loc.locate<i_iterate_active_goals>()),
    get_goal_candidate_rule_ids(loc.locate<i_get_goal_candidate_rule_ids>()),
    rng(rng) {}

const resolution_lineage* random_decision_generator::generate() {
    const goal_lineage* gl = choose_goal();
    rule_id r = choose_candidate(gl);
    return make_resolution_lineage.make_resolution_lineage(gl, r);
}

const goal_lineage* random_decision_generator::choose_goal() {
    std::vector<const goal_lineage*> goals;
    auto it = iterate_active_goals.iterate_active_goals();
    while (!it.done()) {
        it.resume();
        if (!it.has_yield())
            continue;
        goals.push_back(it.consume_yield());
    }

    std::uniform_int_distribution<size_t> dist(0, goals.size() - 1);
    return goals[dist(rng)];
}

rule_id random_decision_generator::choose_candidate(const goal_lineage* gl) {
    std::vector<rule_id> candidates;
    auto& rules = get_goal_candidate_rule_ids.get(gl);
    auto it = rules.iterate();
    while (!it.done()) {
        it.resume();
        if (!it.has_yield())
            continue;
        candidates.push_back(it.consume_yield());
    }

    std::uniform_int_distribution<size_t> dist(0, candidates.size() - 1);
    return candidates[dist(rng)];
}
