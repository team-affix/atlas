#ifndef MAKE_INITIAL_GOAL_LINEAGE_HPP
#define MAKE_INITIAL_GOAL_LINEAGE_HPP

#include "value_objects/lineage.hpp"

template<typename ILineagePool>
struct make_initial_goal_lineage {
    explicit make_initial_goal_lineage(ILineagePool& lp);
    const goal_lineage* make(subgoal_id idx);
private:
    ILineagePool& make_goal_lineage;
};

template<typename ILineagePool>
make_initial_goal_lineage<ILineagePool>::make_initial_goal_lineage(ILineagePool& lp)
    : make_goal_lineage(lp) {}

template<typename ILineagePool>
const goal_lineage* make_initial_goal_lineage<ILineagePool>::make(subgoal_id idx) {
    return make_goal_lineage.make_goal_lineage(nullptr, idx);
}

#endif
