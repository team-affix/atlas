#include "../../hpp/infrastructure/goal_factory.hpp"
#include "../../hpp/domain/value_objects/goal.hpp"

goal_factory::goal_factory() {
}

std::unique_ptr<goal> goal_factory::make(const goal_lineage* gl) const {
    return std::make_unique<goal>(gl);
}
