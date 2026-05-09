#ifndef DECIDER_HPP
#define DECIDER_HPP

#include "../interfaces/i_decider.hpp"
#include "../interfaces/i_decision_generator.hpp"
#include "../interfaces/i_decision_store.hpp"
#include "../interfaces/i_goal_resolver.hpp"

struct decider : i_decider {
    decider();
    void decide() const override;
private:
    i_decision_generator& decision_generator;
    i_decision_store& decision_store;
    i_goal_resolver& goal_resolver;
};

#endif
