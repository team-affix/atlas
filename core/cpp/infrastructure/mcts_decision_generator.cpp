#include "infrastructure/mcts_decision_generator.hpp"

mcts_decision_generator::mcts_decision_generator(
    locator& loc,
    monte_carlo::simulation<mcts_choice, std::mt19937>& sim)
    :
    make_resolution_lineage(loc.locate<i_make_resolution_lineage>()),
    iterate_active_goals(loc.locate<i_iterate_active_goals>()),
    active_goals_size(loc.locate<i_active_goals_size>()),
    get_goal_candidate_rule_ids(loc.locate<i_get_goal_candidate_rule_ids>()),
    sim(sim) {
}

const resolution_lineage* mcts_decision_generator::generate() {
    const goal_lineage* chosen_gl = choose_goal();
    rule_id chosen_r = choose_candidate(chosen_gl);
    return make_resolution_lineage.make_resolution_lineage(chosen_gl, chosen_r);
}

const goal_lineage* mcts_decision_generator::choose_goal() {
    std::vector<mcts_choice> goal_choices;
    goal_choices.reserve(active_goals_size.active_goals_size());

    auto goals = iterate_active_goals.iterate_active_goals();
    while (!goals.done()) {
        goals.resume();
        if (!goals.has_yield())
            continue;
        goal_choices.push_back(goals.consume_yield());
    }

    const mcts_choice choice_a = sim.choose(goal_choices);
    return std::get<const goal_lineage*>(choice_a);
}

rule_id mcts_decision_generator::choose_candidate(const goal_lineage* goal) {
    const auto& candidates = get_goal_candidate_rule_ids.get(goal);

    std::vector<mcts_choice> candidate_choices;
    candidate_choices.reserve(candidates.size());

    auto it = candidates.iterate();
    while (!it.done()) {
        it.resume();
        if (!it.has_yield())
            continue;
        candidate_choices.push_back(it.consume_yield());
    }

    const mcts_choice choice_b = sim.choose(candidate_choices);
    return std::get<rule_id>(choice_b);
}
