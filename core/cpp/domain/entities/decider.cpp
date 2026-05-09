#include "../../../hpp/domain/entities/decider.hpp"
#include "../../../hpp/bootstrap/resolver.hpp"

decider::decider() :
    decision_generator(resolver::resolve<i_decision_generator>()),
    decision_store(resolver::resolve<i_decision_store>()),
    goal_resolver(resolver::resolve<i_goal_resolver>()) {}

void decider::decide() const {
    auto rl = decision_generator.generate();
    decision_store.insert(rl);
    goal_resolver.resolve(rl);
}
