#include <vector>
#include "infrastructure/mcts_decision_generator.hpp"

mcts_decision_generator::mcts_decision_generator(locator& loc)
    :
    make_resolution_lineage(loc.locate<i_make_resolution_lineage>()),
    iterate_root_goals(loc.locate<i_iterate_root_goals>()),
    iterate_child_goals(loc.locate<i_iterate_child_goals>()),
    mcts_choose(loc.locate<i_mcts_choose>()),
    get_goal_candidate_rule_ids(loc.locate<i_get_goal_candidate_rule_ids>()),
    is_active_goal(loc.locate<i_is_active_goal>()) {}


const resolution_lineage* mcts_decision_generator::generate() {
    const goal_lineage* gl = choose_goal();
    rule_id r = choose_candidate(gl);
    return make_resolution_lineage.make_resolution_lineage(gl, r);
}

const goal_lineage* mcts_decision_generator::choose_goal() {

    // current choices
    std::vector<mcts_choice> current;

    // initial state machine
    auto sm = iterate_root_goals.iterate_root_goals();

    // enter a choice loop until reach leaf
    while (true) {
        // compute current
        while (!sm.done()) {
            sm.resume();
            if (!sm.has_yield())
                continue;
            current.push_back(sm.consume_yield());
        }

        // choose a goal
        mcts_choice choice = mcts_choose.choose(current);

        // get the goal_lineage
        const goal_lineage* gl = std::get<const goal_lineage*>(choice);

        // check if the goal is leaf
        if (is_active_goal.is_active_goal(gl))
            return gl;

        // clear current
        current.clear();
        sm = iterate_child_goals.iterate_child_goals(gl);
    }
}

rule_id mcts_decision_generator::choose_candidate(const goal_lineage* gl) {
    auto& rule_ids = get_goal_candidate_rule_ids.get(gl);

    std::vector<mcts_choice> candidates;

    candidates.reserve(rule_ids.size());
    
    auto sm = rule_ids.iterate();

    while (!sm.done()) {
        sm.resume();
        if (!sm.has_yield())
            continue;
        candidates.push_back(sm.consume_yield());
    }
    
    mcts_choice choice = mcts_choose.choose(candidates);

    return std::get<rule_id>(choice);
}
