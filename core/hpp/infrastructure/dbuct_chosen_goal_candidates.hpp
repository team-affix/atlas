#ifndef DBUCT_CHOSEN_GOAL_CANDIDATES_HPP
#define DBUCT_CHOSEN_GOAL_CANDIDATES_HPP

#include <list>
#include <optional>
#include <stack>
#include <unordered_map>
#include "value_objects/chosen_candidate_action.hpp"
#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"
#include "debug_assert.hpp"

struct dbuct_chosen_goal_candidates {
    std::optional<rule_id> try_get(const goal_lineage* gl) const;
    void set(const goal_lineage* gl, rule_id r);

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<chosen_candidate_action> actions;
    };

    using map_t = std::unordered_map<const goal_lineage*, rule_id>;

    void log(chosen_candidate_action action);
    void undo_action(const chosen_candidate_action& action);

    map_t by_goal_;
    std::stack<frame> frame_stack_;
};

#endif
