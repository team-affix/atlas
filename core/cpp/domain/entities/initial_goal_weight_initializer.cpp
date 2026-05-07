#include "../../../hpp/domain/entities/initial_goal_weight_initializer.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

initial_goal_weight_initializer::initial_goal_weight_initializer(size_t goal_count) :
    gws(resolver::resolve<i_goal_weight_store>()),
    initial_weight(1.0 / goal_count) {
}

void initial_goal_weight_initializer::initialize(const goal_lineage* gl) {
    gws.insert(gl, initial_weight);
}
