#include "../../../hpp/domain/entities/decider.hpp"
#include "../../../hpp/bootstrap/locator.hpp"

decider::decider() :
    decision_generator(locator::resolve<i_decision_generator>()),
    decision_memory(locator::resolve<i_decision_memory>()),
    res(locator::resolve<i_resolver>()) {}

void decider::decide() const {
    auto rl = decision_generator.generate();
    decision_memory.insert(rl);
    res.init_resolve(rl);
}
