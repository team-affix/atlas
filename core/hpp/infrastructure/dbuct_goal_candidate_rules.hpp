#ifndef DBUCT_GOAL_CANDIDATE_RULES_HPP
#define DBUCT_GOAL_CANDIDATE_RULES_HPP

#include <deque>
#include <list>
#include <stack>
#include <unordered_map>
#include "infrastructure/ra_rule_id_set_factory.hpp"
#include "infrastructure/ra_rule_id_set.hpp"
#include "value_objects/goal_candidate_rules_action.hpp"
#include "value_objects/lineage.hpp"
#include "debug_assert.hpp"

struct dbuct_goal_candidate_rules {
    dbuct_goal_candidate_rules(ra_rule_id_set_factory& factory);

    const ra_rule_id_set& get(const goal_lineage* gl) const;
    void insert(const goal_lineage* gl);
    void link_goal_candidate(const goal_lineage* gl, rule_id r);
    void unlink_goal_candidate(const goal_lineage* gl, rule_id r);
    void erase(const goal_lineage* gl);

    void push_frame();
    void pop_frame();

private:
    struct frame {
        std::list<goal_candidate_rules_action> actions;
    };

    using map_t = std::unordered_map<const goal_lineage*, ra_rule_id_set>;

    void log(goal_candidate_rules_action action);
    void undo_action(const goal_candidate_rules_action& action);

    ra_rule_id_set_factory& factory_;
    map_t by_goal_;
    std::stack<frame> frame_stack_{std::deque<frame>{frame{}}};
};

#endif
