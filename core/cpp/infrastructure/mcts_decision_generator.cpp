#include "../../hpp/infrastructure/mcts_decision_generator.hpp"

mcts_decision_generator::mcts_decision_generator(
    i_lineage_pool& lp,
    const i_active_goals& ag,
    i_get_goal_candidate_rules& ggcr,
    i_mcts_choice_generator_goal_visitor_factory& gvf,
    i_mcts_choice_generator_candidate_visitor_factory& cvf,
    monte_carlo::simulation<mcts_choice, std::mt19937>& sim)
    :
    lp(lp),
    ag(ag),
    ggcr(ggcr),
    gvf(gvf),
    cvf(cvf),
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

    // Create a visitor for the goals
    auto vis = gvf.make(goal_choices);
    
    // Get the goals to choose from
    ag.accept(*vis);

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

    // Create a visitor for the candidates
    auto vis = cvf.make(candidate_choices);

    // Visit the candidates
    candidates.accept(*vis);

    // Choose a candidate for the goal
    const mcts_choice choice_b = sim.choose(candidate_choices);
    return std::get<const rule*>(choice_b);
}
