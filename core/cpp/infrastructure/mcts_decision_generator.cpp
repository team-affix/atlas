#include "../../hpp/infrastructure/mcts_decision_generator.hpp"

mcts_decision_generator::mcts_decision_generator(
    i_lineage_pool& lp,
    const i_active_goals& ag,
    i_get_goal_candidate_rules& ggcr,
    monte_carlo::simulation<mcts_choice, std::mt19937>& sim)
    :
    lp(lp),
    ag(ag),
    ggcr(ggcr),
    sim(sim) {
}

const resolution_lineage* mcts_decision_generator::generate() {
    const goal_lineage* chosen_gl = choose_goal();
    const rule* chosen_r = choose_candidate(chosen_gl);
    return lp.resolution(chosen_gl, chosen_r);
}

const goal_lineage* mcts_decision_generator::choose_goal() {
    // Reserve space for the goal choices
    std::vector<mcts_choice> goal_choices;
    goal_choices.reserve(ag.size());

    // Get the goals to choose from
    auto goals = ag.iterate();
    while (!goals.done()) {
        auto gl = goals.resume();
        if (!gl.has_value())
            continue;
        goal_choices.push_back(gl.value());
    }

    // Choose a goal to resolve
    const mcts_choice choice_a = sim.choose(goal_choices);
    return std::get<const goal_lineage*>(choice_a);
}

const rule* mcts_decision_generator::choose_candidate(const goal_lineage* goal) {
    // Get the candidates to choose from
    const auto& candidates = ggcr.get(goal);
    
    // Get the candidates to choose from
    std::vector<mcts_choice> candidate_choices;
    candidate_choices.reserve(candidates.size());

    // Visit the candidates
    auto it = candidates.iterate();
    while (!it.done()) {
        auto r = it.resume();
        if (!r.has_value())
            continue;
        candidate_choices.push_back(r.value());
    }

    // Choose a candidate for the goal
    const mcts_choice choice_b = sim.choose(candidate_choices);
    return std::get<const rule*>(choice_b);
}
