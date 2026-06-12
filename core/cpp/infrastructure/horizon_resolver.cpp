#include "infrastructure/horizon_resolver.hpp"

horizon_resolver::horizon_resolver(locator& loc)
    : resolver_(loc.locate<resolver>()),
      get_rule_(loc.locate<i_get_rule>()),
      get_goal_weight_(loc.locate<i_get_goal_weight>()),
      accumulate_grounded_weight_(loc.locate<i_accumulate_grounded_weight>()) {}

bool horizon_resolver::resolve(const resolution_lineage* rl) {
    const rule* rule = get_rule_.get(rl->idx);
    if (rule->body.empty()) {
        const double w = get_goal_weight_.get(rl->parent);
        accumulate_grounded_weight_.accumulate(w);
    }
    return resolver_.resolve(rl);
}
