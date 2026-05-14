#ifndef GOAL_FACTORY_HPP
#define GOAL_FACTORY_HPP

#include <memory>
#include "../domain/value_objects/goal.hpp"
#include "../domain/value_objects/lineage.hpp"
#include "../domain/interfaces/i_factory.hpp"

struct goal_factory : i_factory<goal, const goal_lineage*> {
    goal_factory();
    std::unique_ptr<goal> make(const goal_lineage*) const override;
};

#endif
