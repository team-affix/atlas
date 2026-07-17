#ifndef UNIFORM_RULE_ROLLOUT_HPP
#define UNIFORM_RULE_ROLLOUT_HPP

#include <cstddef>
#include <random>
#include <vector>
#include "value_objects/rule.hpp"

template<typename IRndGen>
struct uniform_rule_rollout {
    uniform_rule_rollout(IRndGen&);
    rule_id rollout_choose_rule(const std::vector<rule_id>& rules);
private:
    IRndGen& rnd_gen_;
};

template<typename IRndGen>
uniform_rule_rollout<IRndGen>::uniform_rule_rollout(IRndGen& rnd_gen)
    : rnd_gen_(rnd_gen) {}

template<typename IRndGen>
rule_id uniform_rule_rollout<IRndGen>::rollout_choose_rule(
    const std::vector<rule_id>& rules) {
    std::uniform_int_distribution<size_t> dist(0, rules.size() - 1);
    return rules[dist(rnd_gen_)];
}

#endif
