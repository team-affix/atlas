#ifndef HORIZON_GOAL_ACTIVATOR_HPP
#define HORIZON_GOAL_ACTIVATOR_HPP

#include "value_objects/lineage.hpp"
#include "value_objects/rule.hpp"

template<typename IGoalActivator, typename ISetGoalWeight, typename IGetRule>
struct horizon_goal_activator {
    horizon_goal_activator(IGoalActivator&, ISetGoalWeight&, IGetRule&);
    void activate(const goal_lineage*);
private:
    IGoalActivator& goal_activator_;
    ISetGoalWeight& goal_weights_;
    IGetRule& get_rule_;
};

template<typename IGA, typename ISGW, typename IGR>
horizon_goal_activator<IGA,ISGW,IGR>::horizon_goal_activator(IGA& ga, ISGW& gw, IGR& db)
    : goal_activator_(ga), goal_weights_(gw), get_rule_(db) {}

template<typename IGA, typename ISGW, typename IGR>
void horizon_goal_activator<IGA,ISGW,IGR>::activate(const goal_lineage* gl) {
    goal_activator_.activate(gl);
    const resolution_lineage* rl = gl->parent;
    const goal_lineage* parent = rl->parent;
    const rule* rule = get_rule_.get_rule(rl->idx);
    const double parent_w = goal_weights_.get(parent);
    const size_t g = rule->body.size();
    goal_weights_.set(gl, parent_w / static_cast<double>(g));
}

#endif
