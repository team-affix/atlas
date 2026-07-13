#ifndef HORIZON_GOAL_ACTIVATOR_HPP
#define HORIZON_GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IGoalActivator, typename IGetGoalWeight, typename ISetGoalWeight, typename IGetRule>
struct horizon_goal_activator {
    horizon_goal_activator(IGoalActivator&, IGetGoalWeight&, ISetGoalWeight&, IGetRule&);
    void activate(const goal_lineage*);
private:
    IGoalActivator& goal_activator_;
    IGetGoalWeight& get_goal_weight_;
    ISetGoalWeight& set_goal_weight_;
    IGetRule& get_rule_;
};

template<typename IGA, typename IGGW, typename ISGW, typename IGR>
horizon_goal_activator<IGA, IGGW, ISGW, IGR>::horizon_goal_activator(
    IGA& ga, IGGW& ggw, ISGW& sgw, IGR& db)
    : goal_activator_(ga), get_goal_weight_(ggw), set_goal_weight_(sgw), get_rule_(db) {}

template<typename IGA, typename IGGW, typename ISGW, typename IGR>
void horizon_goal_activator<IGA, IGGW, ISGW, IGR>::activate(const goal_lineage* gl) {
    goal_activator_.activate(gl);
    const resolution_lineage* rl = gl->parent;
    const goal_lineage* parent = rl->parent;
    const rule* rule = get_rule_.get_rule(rl->idx);
    const double parent_w = get_goal_weight_.get(parent);
    const size_t g = rule->body.size();
    set_goal_weight_.set(gl, parent_w / static_cast<double>(g));
}

#endif
