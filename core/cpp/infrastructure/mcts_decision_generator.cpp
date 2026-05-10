#include "../../hpp/infrastructure/mcts_decision_generator.hpp"
#include "../../hpp/bootstrap/resolver.hpp"
#include "../../hpp/infrastructure/mcts_choice_generator_goal_visitor.hpp"

mcts_decision_generator::mcts_decision_generator()
    :
    lp(resolver::resolve<i_lineage_pool>()),
    ags(resolver::resolve<i_active_goal_store>()),
    gcs(resolver::resolve<i_goal_candidates_store>()),
    sim(resolver::resolve<monte_carlo::simulation<mcts_choice, std::mt19937>>()){
}

const resolution_lineage* mcts_decision_generator::generate() {
    const goal_lineage* chosen_gl = choose_goal();
    const size_t chosen_i = choose_candidate(chosen_gl);
    return lp.resolution(chosen_gl, chosen_i);
}

const goal_lineage* mcts_decision_generator::choose_goal() {
    // Get the goals to choose from
    mcts_choice_generator_goal_visitor visitor(ags.size());
    ags.accept(visitor);

    // Choose a goal to resolve
    const mcts_choice choice_a = sim.choose(visitor.choices());
    return std::get<const goal_lineage*>(choice_a);
}

size_t mcts_decision_generator::choose_candidate(const goal_lineage* goal) {
    // Get the candidates to choose from
    const candidate_set& candidates = gcs.at(goal);
    
    // Get the candidates to choose from
    std::vector<mcts_choice> candidate_choices;
    candidate_choices.reserve(candidates.candidates.size());

    // Convert the candidates to choices
    for (size_t rule_id : candidates.candidates)
        candidate_choices.push_back(rule_id);

    // Choose a candidate for the goal
    const mcts_choice choice_b = sim.choose(candidate_choices);
    return std::get<size_t>(choice_b);
}
