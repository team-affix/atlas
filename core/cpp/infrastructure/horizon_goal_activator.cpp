#include "infrastructure/horizon_goal_activator.hpp"

horizon_goal_activator::horizon_goal_activator(locator& loc)
    : goal_activator_(loc.locate<goal_activator>()),
      set_goal_weight_(loc.locate<i_set_goal_weight>()),
      get_goal_weight_(loc.locate<i_get_goal_weight>()),
      get_rule_(loc.locate<i_get_rule>()) {}

void horizon_goal_activator::activate(const goal_lineage* gl) {
    goal_activator_.activate(gl);
    const resolution_lineage* rl = gl->parent;
    const goal_lineage* parent = rl->parent;
    const rule* rule = get_rule_.get(rl->idx);
    const double parent_w = get_goal_weight_.get(parent);
    const size_t g = rule->body.size();
    set_goal_weight_.set(gl, parent_w / static_cast<double>(g));
}
